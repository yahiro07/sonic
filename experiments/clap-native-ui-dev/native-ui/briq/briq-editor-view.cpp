#include "briq-editor-view.h"

namespace project1_gui_briq {

using namespace briq;

static void render(UiActor &ui, int w, int h) {
  auto root = ui.rootBox(w, h).hCenter();
  root.sub([&] {
    ui.box(400, 300).draw([&](DrawingContext &dc) {
      dc.fillRect(0, 0, 400, 300, 0xff0000ff);
      // dc.drawText("Hello World", 200, 150, "mainFont", 32, 0xffffffff, true);
    });
  });
}

void BriqEditorView::setup() { editorIntegration.setup(render); }

void BriqEditorView::teardown() {}

} // namespace project1_gui_briq