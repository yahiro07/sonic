#include "./microui-editor-view.h"

extern "C" {
#include "../../framework/plugin-view/microui/microui.h"
}

namespace project1_gui {

using namespace sonic_plugin_view_microui;

Color toColor(mu_Color color) {
  return Color{color.r, color.g, color.b, color.a};
}

Rect toRect(mu_Rect rect) { return Rect{rect.x, rect.y, rect.w, rect.h}; }

Point toPoint(mu_Vec2 point) { return Point{point.x, point.y}; }

class EditorView final : public sonic_plugin_view_microui::IMicrouiEditor {
private:
  IWindowRepresentor &window;
  sonic::IControllerFacade &controllerFacade;
  mu_Context context{};
  float sliderValue = 0.5f;
  float sliderValue2 = 0.5f;
  int previousButtons = 0;
  bool isSetup = false;

public:
  explicit EditorView(IWindowRepresentor &window,
                      sonic::IControllerFacade &controllerFacade)
      : window(window), controllerFacade(controllerFacade) {
    mu_init(&context);
    context.text_width = &EditorView::measureTextWidth;
    context.text_height = &EditorView::measureTextHeight;
    context.style->font = this;
  }

  ~EditorView() override { teardown(); }

  void setup() override {
    if (isSetup) {
      return;
    }

    window.timer().start([this]() { onTick(); }, 50);
    window.input().subscribePointer(
        [this](const PointerEvent &event) { onPointer(event); });
    isSetup = true;
    render();
  }

  void teardown() override {
    if (!isSetup) {
      return;
    }

    window.timer().stop();
    window.input().unsubscribePointer();
    isSetup = false;
  }

private:
  static int measureTextWidth(mu_Font font, const char *text, int length) {
    const auto *self = static_cast<const EditorView *>(font);
    return self->window.screen().textWidth(text, length);
  }

  static int measureTextHeight(mu_Font font) {
    const auto *self = static_cast<const EditorView *>(font);
    return self->window.screen().textHeight();
  }

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
                           toMuMouseButton(mask));
      } else {
        mu_input_mouseup(&context, position.x, position.y,
                         toMuMouseButton(mask));
      }
    }
  }

  int toMuMouseButton(int mask) const {
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

  void render() {
    auto &screen = window.screen();
    screen.beginFrame();
    screen.clear(Color{51, 51, 51, 255});

    mu_begin(&context);

    if (mu_begin_window(&context, "Window", mu_rect(50, 50, 220, 170))) {
      if (mu_button(&context, "Button")) {
      }

      if (mu_button(&context, "Button2")) {
      }

      mu_slider_ex(&context, &sliderValue, 0.0f, 1.0f, 0, "%.2f",
                   MU_OPT_ALIGNCENTER);
      mu_slider_ex(&context, &sliderValue2, 0.0f, 1.0f, 0, "%.2f",
                   MU_OPT_ALIGNCENTER);
      mu_label(&context, "Hello microui");
      mu_end_window(&context);
    }

    mu_end(&context);

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
};

sonic_plugin_view_microui::IMicrouiEditor *
createMicrouiEditor(sonic_plugin_view_microui::IWindowRepresentor &window,
                    sonic::IControllerFacade &controllerFacade) {
  return new EditorView(window, controllerFacade);
}

} // namespace project1_gui
