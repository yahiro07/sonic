#include "./window-representor.h"
#include <functional>

extern "C" {
#include "./microui.h"
}

namespace sonic_plugin_view_microui {

class EditorHelper {
public:
  static Color toColor(mu_Color color) {
    return Color{color.r, color.g, color.b, color.a};
  }

  static Rect toRect(mu_Rect rect) {
    return Rect{rect.x, rect.y, rect.w, rect.h};
  }

  static Point toPoint(mu_Vec2 point) { return Point{point.x, point.y}; }

  static void sendMicroUiCommandsToScreen(mu_Context &context,
                                          IDrawingScreen &screen) {
    mu_Command *command = nullptr;
    while (mu_next_command(&context, &command)) {
      switch (command->type) {
      case MU_COMMAND_RECT:
        screen.drawRect(toRect(command->rect.rect),
                        toColor(command->rect.color));
        break;

      case MU_COMMAND_TEXT:
        screen.drawText(command->text.str, toPoint(command->text.pos),
                        toColor(command->text.color));
        break;

      case MU_COMMAND_ICON:
        screen.drawIcon(command->icon.id, toRect(command->icon.rect),
                        toColor(command->icon.color));
        break;

      case MU_COMMAND_CLIP:
        screen.applyClip(toRect(command->clip.rect));
        break;

      default:
        break;
      }
    }
  }

  static int toMuMouseButton(int mask) {
    switch (mask) {
    case PointerButtonRight:
      return MU_MOUSE_RIGHT;
    case PointerButtonMiddle:
      return MU_MOUSE_MIDDLE;
    case PointerButtonLeft:
    default:
      return MU_MOUSE_LEFT;
    }
  }
};

class TextSizeProvider {
  IWindowRepresentor &window;

public:
  TextSizeProvider(IWindowRepresentor &window) : window(window) {}

  void setup(mu_Context &context) {
    context.text_width = &TextSizeProvider::measureTextWidth;
    context.text_height = &TextSizeProvider::measureTextHeight;
    context.style->font = this;
  }

  static int measureTextWidth(mu_Font font, const char *text, int length) {
    const auto *self = static_cast<const TextSizeProvider *>(font);
    return self->window.screen().textWidth(text, length);
  }

  static int measureTextHeight(mu_Font font) {
    const auto *self = static_cast<const TextSizeProvider *>(font);
    return self->window.screen().textHeight();
  }
};

class MicrouiViewAdaptor {
private:
  IWindowRepresentor &window;
  TextSizeProvider textSizeProvider;
  mu_Context &context;
  int previousButtons = 0;
  std::function<void()> render;
  bool isSetup = false;

private:
  void onTick() { render(); }

  void onPointer(const PointerEvent &event) {
    mu_input_mousemove(&context, event.position.x, event.position.y);

    dispatchButtonChanges(previousButtons, event.buttons, event.position);

    previousButtons = event.buttons;
    render();
  }

  void dispatchButtonChanges(int previousButtons, int currentButtons,
                             Point position) {
    for (int mask :
         {PointerButtonLeft, PointerButtonRight, PointerButtonMiddle}) {
      const bool wasDown = (previousButtons & mask) != 0;
      const bool isDown = (currentButtons & mask) != 0;
      if (wasDown == isDown) {
        continue;
      }

      if (isDown) {
        mu_input_mousedown(&context, position.x, position.y,
                           EditorHelper::toMuMouseButton(mask));
      } else {
        mu_input_mouseup(&context, position.x, position.y,
                         EditorHelper::toMuMouseButton(mask));
      }
    }
  }

public:
  MicrouiViewAdaptor(IWindowRepresentor &window, mu_Context &context)
      : window(window), textSizeProvider(window), context(context) {}

  void setup(std::function<void()> render) {
    if (!isSetup) {
      this->render = render;
      textSizeProvider.setup(context);
      window.input().subscribePointer(
          [this](const PointerEvent &event) { onPointer(event); });
      window.timer().start([this]() { onTick(); }, 50);
      isSetup = true;
    }
  }

  void teardown() {
    if (isSetup) {
      render = nullptr;
      window.input().unsubscribePointer();
      window.timer().stop();
      isSetup = false;
    }
  }
};
} // namespace sonic_plugin_view_microui