#pragma once

#include "interfaces.h"
#include "my_clap_1/rootage/plug_basis.h"
#include "my_clap_1/wrapper/parameter_manager.h"
#include "processor_adapter.h"
#include "sonic_common/general/mac_web_view.h"
#include "sonic_common/logic/parameter_builder_impl.h"
#include "sonic_common/logic/parameter_definitions_provider.h"
#include "webview_bridge.h"
#include <CoreFoundation/CFNotificationCenter.h>
#include <cstddef>
#include <optional>

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

class DownstreamEventPort : public IDownstreamEventPort {
private:
  std::map<int, std::function<void(DownstreamEvent &)>>
      downstreamEventListeners;

public:
  int subscribeDownstreamEvent(
      std::function<void(DownstreamEvent &)> callback) override {
    auto id = downstreamEventListeners.size() + 1;
    downstreamEventListeners[id] = callback;
    return id;
  }
  void unsubscribeDownstreamEvent(int subscriptionId) override {
    downstreamEventListeners.erase(subscriptionId);
  }

  void emitDownstreamEvent(DownstreamEvent &e) {
    for (auto &[id, listener] : downstreamEventListeners) {
      listener(e);
    }
  }
};

class UpstreamEventPort : public IUpStreamEventPort {
  sonic_common::ParameterDefinitionsProvider &parameterDefinitionsProvider;
  ParameterManager &parameterManager;
  ProcessorAdapter &processorAdapter;

  void pushUpstreamEvent(UpstreamEvent &event) {
    processorAdapter.pushUpstreamEvent(event);
  }

public:
  UpstreamEventPort(
      sonic_common::ParameterDefinitionsProvider &parameterDefinitionsProvider,
      ParameterManager &parameterManager, ProcessorAdapter &processorAdapter)
      : parameterDefinitionsProvider(parameterDefinitionsProvider),
        parameterManager(parameterManager), processorAdapter(processorAdapter) {
  }

  void applyParameterEditFromUi(std::string identifier, double value,
                                ParameterEditState editState) override {
    printf("setParameterFromUi: %s %f\n", identifier.c_str(), value);
    auto _address =
        parameterDefinitionsProvider.getAddressByIdentifier(identifier);
    if (!_address)
      return;
    auto paramId = static_cast<uint32_t>(*_address);
    parameterManager.setParameter(paramId, value, false);
    UpstreamEvent e{
        .type = UpstreamEventType::parameterApplyEdit,
        .param = {.paramId = paramId, .value = value},
    };
    pushUpstreamEvent(e);
  }
  void requestNoteOn(int noteNumber, double velocity) override {
    UpstreamEvent e{
        .type = UpstreamEventType::noteOnRequest,
        .note = {.noteNumber = noteNumber, .velocity = velocity},
    };
    pushUpstreamEvent(e);
  }
  void requestNoteOff(int noteNumber) override {
    UpstreamEvent e{
        .type = UpstreamEventType::noteOffRequest,
        .note = {.noteNumber = noteNumber, .velocity = 0.0},
    };
    pushUpstreamEvent(e);
  }
};

class PlugBasisImpl : public PlugBasis {
private:
  std::unique_ptr<SynthesizerBase> synth;
  ProcessorAdapter processorAdapter;

  sonic_common::MacWebView *webView = nullptr;

  sonic_common::ParameterDefinitionsProvider parameterDefinitionsProvider;

  ParameterManager parameterManager{parameterDefinitionsProvider};
  UpstreamEventPort upstreamEventPort{parameterDefinitionsProvider,
                                      parameterManager, processorAdapter};
  DownstreamEventPort downstreamEventPort;
  WebViewBridge webViewBridge{parameterManager, upstreamEventPort,
                              downstreamEventPort};

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
      parameterManager.setParameter(item.address, item.defaultValue, false);
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
    return parameterManager.getParameter(id);
  }

  void onMainThread() override {
    DownstreamEvent e;
    while (processorAdapter.popDownstreamEvent(e)) {
      if (e.type == DownstreamEventType::ParameterChange) {
        parameterManager.setParameter(e.param.paramId, e.param.value, true);
      }
      downstreamEventPort.emitDownstreamEvent(e);
    }
  }

  bool guiCreate() override {
    webView = new sonic_common::MacWebView();
    std::string url = synth->getEditorPageUrl();
    webView->loadUrl(url);

    webViewBridge.onWebViewOpen(*webView);
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

  // bool guiSetTransient(const clap_window_t *window) override { return
  // false; } void guiSuggestTitle(const char *title) override {}
  bool guiShow() override {
    // webView->show();
    return true;
  }

  bool guiHide() override { return true; }
};