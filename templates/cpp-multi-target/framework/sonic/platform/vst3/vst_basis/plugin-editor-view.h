#pragma once
#include "pluginterfaces/base/funknown.h"
#include <memory>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <sonic/core/editor-factory-registry.h>
#include <sonic/core/editor-interfaces.h>

namespace vst_basis {

using namespace sonic;
using namespace Steinberg;

static std::unique_ptr<IEditorInstance>
setupEditorInstance(std::string url, IControllerFacade &controllerFacade) {
  auto variantName = EditorFactoryRegistry::getEditorVariantNameFromUrl(url);
  auto editorFactory =
      EditorFactoryRegistry::getInstance()->getEditorFactory(variantName);
  if (!editorFactory) {
    printf("editor factory not found for variant: %s\n", variantName.c_str());
    return nullptr;
  }
  auto editorInstance = editorFactory(controllerFacade);
  if (variantName == "webview") {
    editorInstance->setup(url);
  } else {
    auto loadTargetSpec = url.substr(url.find(":") + 1);
    editorInstance->setup(loadTargetSpec);
  }
  return editorInstance;
}

class PluginEditorView : public Vst::EditorView {
private:
  std::unique_ptr<IEditorInstance> editorInstance;
  ViewRect viewRect{0, 0, 900, 600};

public:
  PluginEditorView(Vst::EditController *controller,
                   IControllerFacade &controllerFacade,
                   std::string editorPageUrl)
      : Vst::EditorView(controller) {
    printf("WebViewEditorView::WebViewEditorView\n");

    editorInstance = setupEditorInstance(editorPageUrl, controllerFacade);
  }

  ~PluginEditorView() override {
    printf("WebViewEditorView::~WebViewEditorView\n");
    editorInstance->teardown();
    editorInstance.reset();
  }

  tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override {
    printf("WebViewEditorView::isPlatformTypeSupported: %s\n", type);
    if (FIDStringsEqual(type, kPlatformTypeNSView)) {
      return kResultTrue;
    }
    return kResultFalse;
  }
  tresult PLUGIN_API attached(void *parent, FIDString type) override {
    printf("WebViewEditorView::attached: %p, %s\n", parent, type);
    if (FIDStringsEqual(type, kPlatformTypeNSView)) {
      editorInstance->attachToParent(parent);
      editorInstance->setFrame(viewRect.left, viewRect.top,
                               viewRect.right - viewRect.left,
                               viewRect.bottom - viewRect.top);
      return kResultOk;
    }
    return kResultFalse;
  }

  tresult PLUGIN_API canResize() override { return kResultTrue; }

  tresult PLUGIN_API getSize(ViewRect *size) override {
    if (!size) {
      return kInvalidArgument;
    }
    *size = viewRect;
    printf("WebViewEditorView::getSize: %d, %d, %d, %d\n", size->left,
           size->top, size->right, size->bottom);
    return kResultOk;
  }
  tresult PLUGIN_API removed() override {
    printf("WebViewEditorView::removed\n");
    if (!editorInstance)
      return kResultFalse;
    editorInstance->removeFromParent();
    return kResultOk;
  }

  tresult PLUGIN_API onSize(ViewRect *newSize) override {
    if (!newSize) {
      return kInvalidArgument;
    }
    printf("WebViewEditorView::onSize: %d, %d, %d, %d\n", newSize->left,
           newSize->top, newSize->right, newSize->bottom);
    viewRect = *newSize;
    if (!editorInstance)
      return kResultFalse;
    editorInstance->setFrame(newSize->left, newSize->top,
                             newSize->right - newSize->left,
                             newSize->bottom - newSize->top);
    return kResultOk;
  }
};
} // namespace vst_basis