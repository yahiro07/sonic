#import <Cocoa/Cocoa.h>

#include "window-representor.h"

#include <utility>
#include <vector>

namespace sonic_plugin_view_microui {

class WindowRepresentorImpl;
}

@interface WindowView : NSView {
  void *owner_;
}
- (instancetype)initWithOwner:(void *)owner frame:(NSRect)frame;
@end

namespace sonic_plugin_view_microui {
namespace {

NSPoint toNSPoint(Point point) { return NSMakePoint(point.x, point.y); }

NSRect toNSRect(Rect rect) {
  return NSMakeRect(rect.x, rect.y, rect.w, rect.h);
}

NSColor *toNSColor(Color color) {
  return [NSColor colorWithCalibratedRed:color.r / 255.0
                                   green:color.g / 255.0
                                    blue:color.b / 255.0
                                   alpha:color.a / 255.0];
}

class CommandBufferScreen final : public IDrawingScreen {
public:
  void beginFrame() override { commands_.clear(); }

  void clear(Color color) override {
    commands_.push_back(
        Command{CommandType::Clear, Rect{}, Point{}, color, 0, {}});
  }

  void drawRect(Rect rect, Color color) override {
    commands_.push_back(
        Command{CommandType::Rect, rect, Point{}, color, 0, {}});
  }

  void drawText(const std::string &text, Point position, Color color) override {
    Command command{CommandType::Text, Rect{}, position, color, 0, text};
    commands_.push_back(std::move(command));
  }

  void drawIcon(int iconId, Rect rect, Color color) override {
    commands_.push_back(
        Command{CommandType::Icon, rect, Point{}, color, iconId, {}});
  }

  void applyClip(Rect rect) override {
    commands_.push_back(
        Command{CommandType::Clip, rect, Point{}, Color{}, 0, {}});
  }

  int textWidth(const char *text, int length) const override {
    if (text == nullptr || length <= 0) {
      return 0;
    }
    return length * 8;
  }

  int textHeight() const override { return 16; }

  void flush(NSView *view) {
    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
    CGContextSaveGState(context);

    for (const Command &command : commands_) {
      switch (command.type) {
      case CommandType::Clear:
        CGContextRestoreGState(context);
        CGContextSaveGState(context);
        CGContextSetFillColorWithColor(context,
                                       toNSColor(command.color).CGColor);
        CGContextFillRect(context, view.bounds);
        break;

      case CommandType::Rect:
        CGContextSetFillColorWithColor(context,
                                       toNSColor(command.color).CGColor);
        CGContextFillRect(context, toNSRect(command.rect));
        break;

      case CommandType::Text: {
        NSDictionary *attributes = @{
          NSFontAttributeName : [NSFont systemFontOfSize:13],
          NSForegroundColorAttributeName : toNSColor(command.color),
        };
        NSString *text = [NSString stringWithUTF8String:command.text.c_str()];
        [text drawAtPoint:toNSPoint(command.position)
            withAttributes:attributes];
        break;
      }

      case CommandType::Icon:
        drawIcon(context, command.icon_id, command.rect, command.color);
        break;

      case CommandType::Clip:
        CGContextRestoreGState(context);
        CGContextSaveGState(context);
        CGContextClipToRect(context, toNSRect(command.rect));
        break;
      }
    }

    CGContextRestoreGState(context);
  }

private:
  enum class CommandType {
    Clear,
    Rect,
    Text,
    Icon,
    Clip,
  };

  struct Command {
    CommandType type;
    Rect rect;
    Point position;
    Color color;
    int icon_id;
    std::string text;
  };

  static void drawIcon(CGContextRef context, int iconId, Rect rect,
                       Color color) {
    CGContextSetFillColorWithColor(context, toNSColor(color).CGColor);

    switch (iconId) {
    case 1: {
      const CGFloat inset = 4.0;
      CGContextSetLineWidth(context, 2.0);
      CGContextMoveToPoint(context, rect.x + inset, rect.y + inset);
      CGContextAddLineToPoint(context, rect.x + rect.w - inset,
                              rect.y + rect.h - inset);
      CGContextMoveToPoint(context, rect.x + rect.w - inset, rect.y + inset);
      CGContextAddLineToPoint(context, rect.x + inset, rect.y + rect.h - inset);
      CGContextStrokePath(context);
      break;
    }

    case 2: {
      CGContextMoveToPoint(context, rect.x + rect.w * 0.15,
                           rect.y + rect.h * 0.55);
      CGContextAddLineToPoint(context, rect.x + rect.w * 0.42,
                              rect.y + rect.h * 0.82);
      CGContextAddLineToPoint(context, rect.x + rect.w * 0.85,
                              rect.y + rect.h * 0.2);
      CGContextSetLineWidth(context, 2.0);
      CGContextStrokePath(context);
      break;
    }

    case 3:
    case 4: {
      CGContextBeginPath(context);
      if (iconId == 3) {
        CGContextMoveToPoint(context, rect.x + rect.w * 0.3,
                             rect.y + rect.h * 0.2);
        CGContextAddLineToPoint(context, rect.x + rect.w * 0.75,
                                rect.y + rect.h * 0.5);
        CGContextAddLineToPoint(context, rect.x + rect.w * 0.3,
                                rect.y + rect.h * 0.8);
      } else {
        CGContextMoveToPoint(context, rect.x + rect.w * 0.2,
                             rect.y + rect.h * 0.3);
        CGContextAddLineToPoint(context, rect.x + rect.w * 0.5,
                                rect.y + rect.h * 0.75);
        CGContextAddLineToPoint(context, rect.x + rect.w * 0.8,
                                rect.y + rect.h * 0.3);
      }
      CGContextClosePath(context);
      CGContextFillPath(context);
      break;
    }

    default:
      CGContextFillRect(context, toNSRect(rect));
      break;
    }
  }

  std::vector<Command> commands_;
};

class InputProvider final : public IInputProvider {
public:
  void subscribePointer(
      std::function<void(const PointerEvent &)> callback) override {
    callback_ = std::move(callback);
  }

  void unsubscribePointer() override { callback_ = nullptr; }

  void emit(const PointerEvent &event) const {
    if (callback_) {
      callback_(event);
    }
  }

private:
  std::function<void(const PointerEvent &)> callback_;
};

class FrameTimer final : public IFrameTimer {
public:
  ~FrameTimer() override { stop(); }

  void setPostTickCallback(std::function<void()> callback) {
    post_tick_callback_ = std::move(callback);
  }

  void start(std::function<void()> callback, int intervalMs) override {
    stop();
    callback_ = std::move(callback);

    timer_ =
        [NSTimer scheduledTimerWithTimeInterval:intervalMs / 1000.0
                                        repeats:YES
                                          block:^(__unused NSTimer *timer) {
                                            if (callback_) {
                                              callback_();
                                            }
                                            if (post_tick_callback_) {
                                              post_tick_callback_();
                                            }
                                          }];
  }

  void stop() override {
    if (timer_ != nil) {
      [timer_ invalidate];
      timer_ = nil;
    }
    callback_ = nullptr;
  }

private:
  NSTimer *timer_ = nil;
  std::function<void()> callback_;
  std::function<void()> post_tick_callback_;
};

} // namespace

class WindowRepresentorImpl {
public:
  WindowRepresentorImpl() {
    timer_.setPostTickCallback([this]() { requestDisplay(); });

    NSRect frame = NSMakeRect(0, 0, 400, 300);
    view_ = [[WindowView alloc] initWithOwner:this frame:frame];
  }

  ~WindowRepresentorImpl() {
    timer_.stop();
    removeFromParent();
  }

  void attachToParent(void *parent) {
    NSView *parentView = (__bridge NSView *)parent;
    if (parentView == nil || view_ == nil) {
      return;
    }

    if (view_.superview != nil && view_.superview != parentView) {
      [view_ removeFromSuperview];
    }

    parent_view_ = parentView;
    if (view_.superview != parentView) {
      [parentView addSubview:view_];
    }

    [view_ setFrame:parentView.bounds];
    [view_ setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [view_ setNeedsDisplay:YES];
  }

  void removeFromParent() {
    current_buttons_ = 0;
    parent_view_ = nil;
    if (view_ != nil && view_.superview != nil) {
      [view_ removeFromSuperview];
    }
  }

  void setFrame(int x, int y, int width, int height) {
    if (view_ == nil) {
      return;
    }
    [view_ setFrame:NSMakeRect(x, y, width, height)];
    [view_ setNeedsDisplay:YES];
  }

  void requestDisplay() { [view_ setNeedsDisplay:YES]; }

  void renderCurrentFrame() { screen_.flush(view_); }

  void handlePointerEvent(PointerEventType type, NSEvent *event,
                          int changedMask) {
    NSPoint location = [view_ convertPoint:event.locationInWindow fromView:nil];

    if (type == PointerEventType::Down) {
      current_buttons_ |= changedMask;
    } else if (type == PointerEventType::Up) {
      current_buttons_ &= ~changedMask;
    }

    input_.emit(PointerEvent{type, current_buttons_,
                             Point{(int)location.x, (int)location.y}});
    requestDisplay();
  }

  InputProvider input_;
  CommandBufferScreen screen_;
  FrameTimer timer_;

private:
  NSView *parent_view_ = nil;
  WindowView *view_ = nil;
  int current_buttons_ = 0;
};

} // namespace sonic_plugin_view_microui

@implementation WindowView

- (instancetype)initWithOwner:(void *)owner frame:(NSRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    owner_ = owner;
  }
  return self;
}

- (BOOL)isFlipped {
  return YES;
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  if (self.window) {
    self.window.acceptsMouseMovedEvents = YES;
    [self.window makeFirstResponder:self];
  }
}

- (void)updateTrackingAreas {
  [super updateTrackingAreas];

  NSArray<NSTrackingArea *> *areas = [self.trackingAreas copy];
  for (NSTrackingArea *area in areas) {
    [self removeTrackingArea:area];
  }

  NSTrackingAreaOptions options = NSTrackingMouseMoved |
                                  NSTrackingActiveInKeyWindow |
                                  NSTrackingInVisibleRect;
  NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                      options:options
                                                        owner:self
                                                     userInfo:nil];
  [self addTrackingArea:area];
}

using namespace sonic_plugin_view_microui;

- (void)mouseMoved:(NSEvent *)event {
  static_cast<WindowRepresentorImpl *>(owner_)->handlePointerEvent(
      PointerEventType::Move, event, 0);
}

- (void)mouseDragged:(NSEvent *)event {
  [self mouseMoved:event];
}

- (void)rightMouseDragged:(NSEvent *)event {
  [self mouseMoved:event];
}

- (void)otherMouseDragged:(NSEvent *)event {
  [self mouseMoved:event];
}

- (void)mouseDown:(NSEvent *)event {
  static_cast<WindowRepresentorImpl *>(owner_)->handlePointerEvent(
      PointerEventType::Down, event, PointerButtonLeft);
}

- (void)mouseUp:(NSEvent *)event {
  static_cast<WindowRepresentorImpl *>(owner_)->handlePointerEvent(
      PointerEventType::Up, event, PointerButtonLeft);
}

- (void)rightMouseDown:(NSEvent *)event {
  static_cast<WindowRepresentorImpl *>(owner_)->handlePointerEvent(
      PointerEventType::Down, event, PointerButtonRight);
}

- (void)rightMouseUp:(NSEvent *)event {
  static_cast<WindowRepresentorImpl *>(owner_)->handlePointerEvent(
      PointerEventType::Up, event, PointerButtonRight);
}

- (void)otherMouseDown:(NSEvent *)event {
  static_cast<WindowRepresentorImpl *>(owner_)->handlePointerEvent(
      PointerEventType::Down, event, PointerButtonMiddle);
}

- (void)otherMouseUp:(NSEvent *)event {
  static_cast<WindowRepresentorImpl *>(owner_)->handlePointerEvent(
      PointerEventType::Up, event, PointerButtonMiddle);
}

- (void)drawRect:(__unused NSRect)dirtyRect {
  static_cast<WindowRepresentorImpl *>(owner_)->renderCurrentFrame();
}

@end

namespace sonic_plugin_view_microui {

struct WindowRepresentor::Impl : WindowRepresentorImpl {};

WindowRepresentor::WindowRepresentor() : impl_(std::make_unique<Impl>()) {}

WindowRepresentor::~WindowRepresentor() = default;

IInputProvider &WindowRepresentor::input() { return impl_->input_; }

IDrawingScreen &WindowRepresentor::screen() { return impl_->screen_; }

IFrameTimer &WindowRepresentor::timer() { return impl_->timer_; }

void WindowRepresentor::attachToParent(void *parent) {
  impl_->attachToParent(parent);
}

void WindowRepresentor::removeFromParent() { impl_->removeFromParent(); }

void WindowRepresentor::setFrame(int x, int y, int width, int height) {
  impl_->setFrame(x, y, width, height);
}

} // namespace sonic_plugin_view_microui