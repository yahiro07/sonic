#include "editor-view.h"

extern "C" {
#include "microui.h"
}

namespace sonic_plugin_view_microui {
namespace {

Color toColor(mu_Color color) {
  return Color{color.r, color.g, color.b, color.a};
}

Rect toRect(mu_Rect rect) { return Rect{rect.x, rect.y, rect.w, rect.h}; }

Point toPoint(mu_Vec2 point) { return Point{point.x, point.y}; }

class EditorView final : public IEditor {
public:
  explicit EditorView(IWindowRepresentor &window) : window_(window) {
    mu_init(&context_);
    context_.text_width = &EditorView::measureTextWidth;
    context_.text_height = &EditorView::measureTextHeight;
    context_.style->font = this;
  }

  ~EditorView() override { teardown(); }

  void setup() override {
    if (is_setup_) {
      return;
    }

    window_.timer().start([this]() { onTick(); }, 50);
    window_.input().subscribePointer(
        [this](const PointerEvent &event) { onPointer(event); });
    is_setup_ = true;
    render();
  }

  void teardown() override {
    if (!is_setup_) {
      return;
    }

    window_.timer().stop();
    window_.input().unsubscribePointer();
    is_setup_ = false;
  }

private:
  static int measureTextWidth(mu_Font font, const char *text, int length) {
    const auto *self = static_cast<const EditorView *>(font);
    return self->window_.screen().textWidth(text, length);
  }

  static int measureTextHeight(mu_Font font) {
    const auto *self = static_cast<const EditorView *>(font);
    return self->window_.screen().textHeight();
  }

  void onTick() { render(); }

  void onPointer(const PointerEvent &event) {
    mu_input_mousemove(&context_, event.position.x, event.position.y);

    dispatchButtonChanges(previous_buttons_, event.buttons, event.position);

    previous_buttons_ = event.buttons;
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
        mu_input_mousedown(&context_, position.x, position.y,
                           toMuMouseButton(mask));
      } else {
        mu_input_mouseup(&context_, position.x, position.y,
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
    auto &screen = window_.screen();
    screen.beginFrame();
    screen.clear(Color{51, 51, 51, 255});

    mu_begin(&context_);

    if (mu_begin_window(&context_, "Window", mu_rect(50, 50, 220, 170))) {
      if (mu_button(&context_, "Button")) {
      }

      if (mu_button(&context_, "Button2")) {
      }

      mu_slider_ex(&context_, &slider_value_, 0.0f, 1.0f, 0, "%.2f",
                   MU_OPT_ALIGNCENTER);
      mu_slider_ex(&context_, &slider_value2_, 0.0f, 1.0f, 0, "%.2f",
                   MU_OPT_ALIGNCENTER);
      mu_label(&context_, "Hello microui");
      mu_end_window(&context_);
    }

    mu_end(&context_);

    mu_Command *command = nullptr;
    while (mu_next_command(&context_, &command)) {
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

  IWindowRepresentor &window_;
  mu_Context context_{};
  float slider_value_ = 0.5f;
  float slider_value2_ = 0.5f;
  int previous_buttons_ = 0;
  bool is_setup_ = false;
};

} // namespace

std::unique_ptr<IEditor> createEditor(IWindowRepresentor &window) {
  return std::make_unique<EditorView>(window);
}

} // namespace sonic_plugin_view_microui