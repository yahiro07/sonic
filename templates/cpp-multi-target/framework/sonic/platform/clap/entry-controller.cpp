#include "./entry-controller.h"
#include "./clap-data-helper.h"
#include "./domain-controller.h"
#include "./processor-adapter.h"
#include <cassert>
#include <sonic/core/editor-factory-registry.h>
#include <sonic/core/parameter-builder-impl.h>

namespace sonic {

class HostCallbackRequester : public IHostCallbackRequester {
private:
  const clap_host_t *host;
  const clap_host_params_t *hostParams;

public:
  void initialize(const clap_host_t *host,
                  const clap_host_params_t *hostParams) {
    this->host = host;
    this->hostParams = hostParams;
  }

  void requestMainThreadCallback() override {
    if (host && host->request_callback) {
      host->request_callback(host);
    }
  }

  void requestFlush() override {
    if (host && hostParams && hostParams->request_flush) {
      hostParams->request_flush(host);
    }
  }
};

class EntryControllerImpl : public EntryController {
private:
  std::unique_ptr<IPluginSynthesizer> synth;
  HostCallbackRequester hostCallbackRequester;
  Eventbridge eventBridge{hostCallbackRequester};
  ProcessorAdapter processorAdapter{*synth, eventBridge};
  DomainController domainController{*synth, eventBridge};
  std::unique_ptr<IEditorInstance> editorInstance;

public:
  EntryControllerImpl(IPluginSynthesizer *synth) : synth(std::move(synth)) {}

  void initialize() override {
    printf("EntryControllerImpl::initialize called\n");

    hostCallbackRequester.initialize(host, hostParams);

    ParameterBuilderImpl builder;
    synth->setupParameters(builder);
    auto parameterItems = builder.getItems();
    domainController.initialize(parameterItems);
  }

  ~EntryControllerImpl() override {
    domainController.teardown();
    guiDestroy();
  }

  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) override {
    processorAdapter.prepareProcessing(sampleRate, maxFrameCount);
  }

  clap_process_status process(const clap_process_t *process) override {
    auto res = processorAdapter.process(process);
    if (res == CLAP_PROCESS_CONTINUE) {
      // telemetrySupport.process();
    }
    return res;
  }

  void flush(const clap_input_events_t *in,
             const clap_output_events_t *out) override {
    processorAdapter.flushParameters(in, out);
  }

  uint32_t getParameterCount() const override {
    return domainController.parametersRegistry.getParameterItems().size();
  }

  void getParameterInfo(uint32_t index,
                        clap_param_info_t *info) const override {
    auto parameterItems =
        domainController.parametersRegistry.getParameterItems();
    auto &item = parameterItems[index];
    assignParameterInfo(info, item);
  }

  double getParameterValue(clap_id id) override {
    return domainController.parameterStore.get(id);
  }

  void onTimer(clap_id timerId) override {
    // printf("onTimer called, timerId: %d\n", timerId);
    // if (timerId == telemetryUpdateTimerId) {
    //   telemetrySupport.updateTelemetries();
    // }
  }

  void onMainThread() override {
    eventBridge.clearMainThreadRequestedFlag();
    domainController.onMainThread();
  }

  bool guiCreate() override {
    std::string url = synth->getEditorPageUrl();
    auto variantName = EditorFactoryRegistry::getEditorVariantNameFromUrl(url);
    auto editorFactory =
        EditorFactoryRegistry::getInstance()->getEditorFactory(variantName);
    if (!editorFactory) {
      printf("editor factory not found for variant: %s\n", variantName.c_str());
      return false;
    }
    editorInstance = editorFactory(domainController.controllerFacade);
    if (variantName == "webview") {
      editorInstance->setup(url);
    } else {
      auto loadTargetSpec = url.substr(url.find(":") + 1);
      editorInstance->setup(loadTargetSpec);
    }
    return true;
  }

  void guiDestroy() override {
    if (editorInstance) {
      editorInstance->removeFromParent();
      editorInstance->teardown();
      editorInstance.reset();
    }
  }

  bool guiSetParent(const clap_window_t *window) override {
    if (!editorInstance) {
      return false;
    }
    assert(0 == std::strcmp(window->api, CLAP_WINDOW_API_COCOA));
    editorInstance->attachToParent(window->cocoa);
    return true;
  }

  bool guiSetSize(uint32_t width, uint32_t height) override {
    if (!editorInstance) {
      return false;
    }
    editorInstance->setFrame(0, 0, width, height);
    return true;
  }

  bool guiShow() override { return true; }

  bool guiHide() override { return true; }
};

EntryController *EntryController::create(void *synthInstance) {
  return new EntryControllerImpl(
      static_cast<sonic::IPluginSynthesizer *>(synthInstance));
}

} // namespace sonic