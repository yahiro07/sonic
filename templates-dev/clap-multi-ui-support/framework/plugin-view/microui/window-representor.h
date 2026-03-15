#pragma once

#include <functional>
#include <memory>
#include <string>

namespace sonic_plugin_view_microui {

struct Point {
  int x;
  int y;
};

struct Rect {
  int x;
  int y;
  int w;
  int h;
};

struct Color {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};

enum PointerButtons {
  PointerButtonLeft = 1 << 0,
  PointerButtonRight = 1 << 1,
  PointerButtonMiddle = 1 << 2,
};

enum class PointerEventType {
  Down,
  Move,
  Up,
};

struct PointerEvent {
  PointerEventType type;
  int buttons;
  Point position;
};

class IInputProvider {
public:
  virtual ~IInputProvider() = default;
  virtual void
  subscribePointer(std::function<void(const PointerEvent &)> callback) = 0;
  virtual void unsubscribePointer() = 0;
};

class IDrawingScreen {
public:
  virtual ~IDrawingScreen() = default;
  virtual void beginFrame() = 0;
  virtual void clear(Color color) = 0;
  virtual void drawRect(Rect rect, Color color) = 0;
  virtual void drawText(const std::string &text, Point position,
                        Color color) = 0;
  virtual void drawIcon(int iconId, Rect rect, Color color) = 0;
  virtual void applyClip(Rect rect) = 0;
  virtual int textWidth(const char *text, int length) const = 0;
  virtual int textHeight() const = 0;
};

class IFrameTimer {
public:
  virtual ~IFrameTimer() = default;
  virtual void start(std::function<void()> callback, int intervalMs) = 0;
  virtual void stop() = 0;
};

class IWindowRepresentor {
public:
  virtual ~IWindowRepresentor() = default;
  virtual IInputProvider &input() = 0;
  virtual IDrawingScreen &screen() = 0;
  virtual IFrameTimer &timer() = 0;
  virtual void runEventLoop() = 0;

  virtual void attachToParent(void *parent) = 0;
  virtual void removeFromParent() = 0;
  virtual void setFrame(int x, int y, int width, int height) = 0;
};

class WindowRepresentor final : public IWindowRepresentor {
public:
  WindowRepresentor();
  ~WindowRepresentor() override;

  IInputProvider &input() override;
  IDrawingScreen &screen() override;
  IFrameTimer &timer() override;
  void runEventLoop() override;

  void attachToParent(void *parent) override;
  void removeFromParent() override;
  void setFrame(int x, int y, int width, int height) override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace sonic_plugin_view_microui
