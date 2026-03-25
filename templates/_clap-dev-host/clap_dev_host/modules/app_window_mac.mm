#include "./app_window_mac.h"
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <dispatch/dispatch.h>
#include <stdio.h>

@interface AppWindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign)
    std::function<void(const std::string &)> *midiSelectionCallback;
@property(nonatomic, assign)
    std::function<void(const std::string &)> *audioSelectionCallback;
@property(nonatomic, assign)
    std::function<void(int, int)> *windowResizeCallback;
@property(nonatomic, assign) bool *suppressWindowResizeCallback;
- (void)midiDeviceSelected:(NSMenuItem *)sender;
- (void)audioDeviceSelected:(NSMenuItem *)sender;
@end

@implementation AppWindowDelegate
- (BOOL)windowShouldClose:(NSWindow *)sender {
  [NSApp stop:nil];
  // Post a dummy event to wake up the run loop so it can exit
  NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                      location:NSMakePoint(0, 0)
                                 modifierFlags:0
                                     timestamp:0.0
                                  windowNumber:0
                                       context:nil
                                       subtype:0
                                         data1:0
                                         data2:0];
  [NSApp postEvent:event atStart:YES];
  return YES;
}

- (void)windowDidResize:(NSNotification *)notification {
  if (!self.windowResizeCallback || !*self.windowResizeCallback) {
    return;
  }
  if (self.suppressWindowResizeCallback && *self.suppressWindowResizeCallback) {
    return;
  }
  NSWindow *window = (NSWindow *)notification.object;
  if (!window) {
    return;
  }
  NSSize sz = window.contentView.frame.size;
  (*self.windowResizeCallback)((int)sz.width, (int)sz.height);
}

- (void)midiDeviceSelected:(NSMenuItem *)sender {
  if (self.midiSelectionCallback && *self.midiSelectionCallback) {
    (*self.midiSelectionCallback)([sender.representedObject UTF8String]);
  }
}

- (void)audioDeviceSelected:(NSMenuItem *)sender {
  if (self.audioSelectionCallback && *self.audioSelectionCallback) {
    (*self.audioSelectionCallback)([sender.representedObject UTF8String]);
  }
}
@end

namespace clap_dev_host {

struct AppWindowMac::InternalStates {
  void *window{nullptr};
  void *delegate{nullptr};
  std::function<void(const std::string &)> midi_selection_callback;
  std::function<void(const std::string &)> audio_selection_callback;
  std::function<void(int, int)> window_resize_callback;
  bool suppress_window_resize_callback{false};
};

AppWindowMac::AppWindowMac() : states(std::make_unique<InternalStates>()) {}

AppWindowMac::~AppWindowMac() {
  @autoreleasepool {
    if (states->delegate) {
      AppWindowDelegate *delegate = (AppWindowDelegate *)states->delegate;
      [delegate release];
    }
    if (states->window) {
      NSWindow *window = (NSWindow *)states->window;
      [window close];
    }
  }
}

void AppWindowMac::show() {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    if (!states->delegate) {
      states->delegate = [[AppWindowDelegate alloc] init];
    }

    // Set pointers to callbacks in InternalStates
    AppWindowDelegate *delegate = (AppWindowDelegate *)states->delegate;
    delegate.midiSelectionCallback = &states->midi_selection_callback;
    delegate.audioSelectionCallback = &states->audio_selection_callback;
    delegate.windowResizeCallback = &states->window_resize_callback;
    delegate.suppressWindowResizeCallback =
        &states->suppress_window_resize_callback;

    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSWindow *window =
        [[NSWindow alloc] initWithContentRect:frame
                                    styleMask:(NSWindowStyleMaskTitled |
                                               NSWindowStyleMaskClosable |
                                               NSWindowStyleMaskResizable)
                                      backing:NSBackingStoreBuffered
                                        defer:NO];

    [window setTitle:@"Clap Dev Host"];
    [window center];
    NSView *contentView = [[NSView alloc]
        initWithFrame:NSMakeRect(0, 0, frame.size.width, frame.size.height)];
    contentView.wantsLayer = YES;
    contentView.layer.backgroundColor =
        [[NSColor windowBackgroundColor] CGColor];

    window.contentView = contentView;
    window.delegate = (id<NSWindowDelegate>)states->delegate;
    [window makeKeyAndOrderFront:nil];

    [NSApp activateIgnoringOtherApps:YES];

    states->window = window;
  }
}

void *AppWindowMac::getViewHandle() {
  NSWindow *window = (NSWindow *)states->window;
  return window.contentView;
}

void AppWindowMac::setWindowSize(int width, int height) {
  @autoreleasepool {
    if (!states->window) {
      return;
    }
    NSWindow *window = (NSWindow *)states->window;

    // Prevent re-entrant callbacks when we change the size programmatically
    // (e.g. responding to plugin-initiated resize requests).
    states->suppress_window_resize_callback = true;
    [window setContentSize:NSMakeSize((CGFloat)width, (CGFloat)height)];

    // Clear suppression on the next tick.
    bool *suppressFlag = &states->suppress_window_resize_callback;
    dispatch_async(dispatch_get_main_queue(), ^{
      *suppressFlag = false;
    });
  }
}

void AppWindowMac::subscribeWindowResize(
    std::function<void(int width, int height)> callback) {
  states->window_resize_callback = callback;
}

void AppWindowMac::unsubscribeWindowResize() {
  states->window_resize_callback = nullptr;
}

void AppWindowMac::loop() {
  @autoreleasepool {
    [NSApp run];
  }
}

static NSMenu *buildMainMenu() {
  NSMenu *mainMenu = [[NSMenu alloc] init];

  // macOS requires the first menu to be the "Application" menu
  NSMenuItem *appMenuItem = [[NSMenuItem alloc] initWithTitle:@""
                                                       action:nil
                                                keyEquivalent:@""];
  [mainMenu addItem:appMenuItem];
  NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@""];
  appMenuItem.submenu = appMenu;

  [appMenu
      addItemWithTitle:[NSString stringWithFormat:@"Quit %@",
                                                  [[NSProcessInfo processInfo]
                                                      processName]]
                action:@selector(terminate:)
         keyEquivalent:@"q"];
  return mainMenu;
}

void AppWindowMac::refreshMidiInputDeviceListMenu(
    const std::vector<MidiDeviceInfo> &devices,
    const std::string &selectedDeviceKey) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    NSMenu *mainMenu = [NSApp mainMenu];
    if (!mainMenu) {
      mainMenu = buildMainMenu();
      [NSApp setMainMenu:mainMenu];
    }

    NSString *menuName = @"MIDI Input";
    NSMenuItem *midiMenuItem = [mainMenu itemWithTitle:menuName];
    if (!midiMenuItem) {
      midiMenuItem = [[NSMenuItem alloc] initWithTitle:menuName
                                                action:nil
                                         keyEquivalent:@""];
      [mainMenu addItem:midiMenuItem];
    }

    NSMenu *midiMenu = [[NSMenu alloc] initWithTitle:menuName];
    midiMenuItem.submenu = midiMenu;

    for (const auto &device : devices) {
      NSMenuItem *item = [[NSMenuItem alloc]
          initWithTitle:[NSString
                            stringWithUTF8String:device.displayName.c_str()]
                 action:@selector(midiDeviceSelected:)
          keyEquivalent:@""];
      item.target = (id)states->delegate;
      item.representedObject =
          [NSString stringWithUTF8String:device.deviceKey.c_str()];
      if (device.deviceKey == selectedDeviceKey) {
        [item setState:NSControlStateValueOn];
      }
      [midiMenu addItem:item];
    }
  }
}

void AppWindowMac::subscribeMidiInputDeviceSelection(
    std::function<void(const std::string &deviceKey)> callback) {
  states->midi_selection_callback = callback;
}

void AppWindowMac::unsubscribeMidiInputDeviceSelection() {
  states->midi_selection_callback = nullptr;
}

void AppWindowMac::refreshAudioDeviceListMenu(
    const std::vector<AudioDeviceInfo> &devices,
    const std::string &selectedDeviceKey) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    NSMenu *mainMenu = [NSApp mainMenu];
    if (!mainMenu) {
      mainMenu = buildMainMenu();
      [NSApp setMainMenu:mainMenu];
    }

    NSString *menuName = @"Audio Device";
    NSMenuItem *audioMenuItem = [mainMenu itemWithTitle:menuName];
    if (!audioMenuItem) {
      audioMenuItem = [[NSMenuItem alloc] initWithTitle:menuName
                                                 action:nil
                                          keyEquivalent:@""];
      [mainMenu addItem:audioMenuItem];
    }

    NSMenu *audioMenu = [[NSMenu alloc] initWithTitle:menuName];
    audioMenuItem.submenu = audioMenu;

    for (const auto &device : devices) {
      NSMenuItem *item = [[NSMenuItem alloc]
          initWithTitle:[NSString
                            stringWithUTF8String:device.displayName.c_str()]
                 action:@selector(audioDeviceSelected:)
          keyEquivalent:@""];
      item.target = (id)states->delegate;
      item.representedObject =
          [NSString stringWithUTF8String:device.deviceKey.c_str()];
      if (device.deviceKey == selectedDeviceKey) {
        [item setState:NSControlStateValueOn];
      }
      [audioMenu addItem:item];
    }
  }
}

void AppWindowMac::subscribeAudioDeviceSelection(
    std::function<void(const std::string &deviceKey)> callback) {
  states->audio_selection_callback = callback;
}

void AppWindowMac::unsubscribeAudioDeviceSelection() {
  states->audio_selection_callback = nullptr;
}

void mac_stop_app() {
  @autoreleasepool {
    [NSApp stop:nil];
    // Post a dummy event to wake up the run loop so it can exit
    NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                        location:NSMakePoint(0, 0)
                                   modifierFlags:0
                                       timestamp:0.0
                                    windowNumber:0
                                         context:nil
                                         subtype:0
                                           data1:0
                                           data2:0];
    [NSApp postEvent:event atStart:YES];
  }
}

} // namespace clap_dev_host