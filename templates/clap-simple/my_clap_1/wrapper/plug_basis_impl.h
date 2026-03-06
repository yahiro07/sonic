#pragma once

#include "messaging_hub.h"
#include "my_clap_1/rootage/plug_basis.h"
#include "processor_adapter.h"
#include "sonic_common/general/mac_web_view.h"
#include "sonic_common/logic/parameter_builder_impl.h"
#include "sonic_common/logic/parameter_definitions_provider.h"
#include <mutex>

#define GUI_API CLAP_WINDOW_API_COCOA

static clap_param_info_flags mapParameterFlags(ParameterFlags flags) {
  clap_param_info_flags clapFlags = 0;
  if (flags & ParameterFlags::IsReadOnly) {
    clapFlags |= CLAP_PARAM_IS_READONLY;
  }
  if (flags & ParameterFlags::IsHidden) {
    clapFlags |= CLAP_PARAM_IS_HIDDEN;
  }
  if (!(flags & ParameterFlags::NonAutomatable)) {
    clapFlags |= CLAP_PARAM_IS_AUTOMATABLE;
  }
  return clapFlags;
}

static void assignParameterInfo(clap_param_info_t *info,
                                const sonic_common::ParameterItem &item) {
  info->id = item.address;
  info->flags = mapParameterFlags(item.flags);
  info->min_value = item.minValue;
  info->max_value = item.maxValue;
  info->default_value = item.defaultValue;
  snprintf(info->name, sizeof(info->name), "%s", item.label.c_str());
}

class PlugBasisImpl : public PlugBasis {
private:
  std::unique_ptr<SynthesizerBase> synth;
  ProcessorAdapter processorAdapter;

  sonic_common::MacWebView *webView = nullptr;

  sonic_common::ParameterDefinitionsProvider parameterDefinitionsProvider;

  std::unordered_map<uint32_t, double> parameterValuesInMainThread;
  mutable std::mutex parameterValuesMutex;

  void setParameterInMainThread(uint32_t paramId, double value) {
    std::scoped_lock lock(parameterValuesMutex);
    parameterValuesInMainThread[paramId] = value;
  }

  double getParameterInMainThread(uint32_t paramId) const {
    std::scoped_lock lock(parameterValuesMutex);
    auto it = parameterValuesInMainThread.find(paramId);
    if (it == parameterValuesInMainThread.end()) {
      return 0.0;
    }
    return it->second;
  }

public:
  PlugBasisImpl(SynthesizerBase &synth)
      : synth(&synth), processorAdapter(this->synth.get()) {}

  void initialize() override {
    auto parameterBuilder = sonic_common::ParameterBuilderImpl();
    synth->setupParameters(parameterBuilder);
    auto parameterItems = parameterBuilder.getItems();
    uint64_t maxAddress = 0xFFFFFFFE; // for valid clap_id
    parameterDefinitionsProvider.addParameters(parameterItems, maxAddress);
    for (auto &item : parameterItems) {
      synth->setParameter(item.address, item.defaultValue);
      setParameterInMainThread(item.address, item.defaultValue);
    }
    processorAdapter.initialize(this->host, this->hostParams);
  }

  ~PlugBasisImpl() override { guiDestroy(); }

  void setSampleRate(double sampleRate) override {
    processorAdapter.setSampleRate(sampleRate);
  }

  clap_process_status process(const clap_process_t *process) override {
    return processorAdapter.process(process);
  }

  void flushParameters(const clap_input_events_t *in,
                       const clap_output_events_t *out) override {
    processorAdapter.flushParameters(in, out);
  }

  uint32_t getParameterCount() const override {
    return parameterDefinitionsProvider.getParameterItems().size();
  }

  void getParameterInfo(uint32_t index,
                        clap_param_info_t *info) const override {
    auto parameterItems = parameterDefinitionsProvider.getParameterItems();
    auto &item = parameterItems[index];
    assignParameterInfo(info, item);
  }

  double getParameterValue(clap_id id) const override {
    return getParameterInMainThread(id);
  }

  void onMainThread() override {
    DownstreamEvent e;
    while (processorAdapter.popDownstreamEvent(e)) {
      if (e.type == DownStreamEventType::parameterChange) {
        setParameterInMainThread(e.param.paramId, e.param.value);
      }
      if (webView) {
        messagingHub_dev_handleEventFromHost(
            e,
            [this](std::string &message) {
              this->webView->sendMessage(message);
            },
            parameterDefinitionsProvider);
      }
    }
  }

  bool guiCreate() override {
    webView = new sonic_common::MacWebView();
    std::string url = synth->getEditorPageUrl();
    webView->loadUrl(url);

    webView->setMessageReceiver([this](const std::string &message) {
      auto performParameterEditFromUi = [this](std::string &identifier,
                                               double value) {
        printf("setParameterFromUi: %s %f\n", identifier.c_str(), value);
        auto _address =
            parameterDefinitionsProvider.getAddressByIdentifier(identifier);
        if (!_address)
          return;
        auto paramId = static_cast<uint32_t>(*_address);
        setParameterInMainThread(paramId, value);
        UpstreamEvent e{
            .type = UpStreamEventType::parameterApplyEdit,
            .param = {.paramId = paramId, .value = value},
        };
        processorAdapter.pushUpstreamEvent(e);
      };
      auto emitUpstreamEvent = [this](UpstreamEvent &event) {
        processorAdapter.pushUpstreamEvent(event);
      };
      messagingHub_dev_handleMessageFromUi(message, performParameterEditFromUi,
                                           emitUpstreamEvent);
    });
    return true;
  }

  void guiDestroy() override {
    if (webView) {
      webView->setMessageReceiver(nullptr);
      webView->removeFromParent();
      delete webView;
      webView = nullptr;
    }
  }

  bool guiSetParent(const clap_window_t *window) override {
    assert(0 == strcmp(window->api, GUI_API));
    webView->attachToParent(window->cocoa);
    return true;
  }

  bool guiSetSize(uint32_t width, uint32_t height) override {
    webView->setFrame(0, 0, width, height);
    return true;
  }

  // bool guiSetTransient(const clap_window_t *window) override { return false;
  // } void guiSuggestTitle(const char *title) override {}
  bool guiShow() override {
    // webView->show();
    return true;
  }

  bool guiHide() override { return true; }
};