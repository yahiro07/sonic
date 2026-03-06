#pragma once

#include "../portable/downstream_event_port.h"
#include "../portable/parameter_manager.h"
#include "../portable/upstream_event_port.h"
#include "./clap_data_helper.h"
#include "./entry_controller_interface.h"
#include "./processor_adapter.h"
#include "my_clap_1/portable/event_bridge.h"
#include "my_clap_1/portable/webview_bridge.h"
#include "sonic_common/general/mac_web_view.h"
#include "sonic_common/logic/parameter_definitions_provider.h"
#include <CoreFoundation/CFNotificationCenter.h>
#include <cstddef>
#include <memory>

#define GUI_API CLAP_WINDOW_API_COCOA

class EntryController : public IEntryController {
private:
  std::unique_ptr<SynthesizerBase> synth;
  ProcessorAdapter processorAdapter;
  Eventbridge eventBridge;

  std::unique_ptr<sonic_common::MacWebView> webView;

  sonic_common::ParameterDefinitionsProvider parameterDefinitionsProvider;

  ParameterManager parameterManager{parameterDefinitionsProvider};
  UpstreamEventPort upstreamEventPort{parameterDefinitionsProvider,
                                      parameterManager, eventBridge};
  DownstreamEventPort downstreamEventPort;
  WebViewBridge webViewBridge{parameterManager, upstreamEventPort,
                              downstreamEventPort};

  std::atomic<bool> mainThreadRequestedFlag = false;

  void setupEventBridgeCallbacks() {
    eventBridge.setUpstreamEventPushCallback([this]() {
      // after pushed an upstream event (from main thread),
      // request host to call flush() for emitting events
      if (this->hostParams) {
        this->hostParams->request_flush(this->host);
      }
    });
    eventBridge.setDownstreamEventPushCallback([this]() {
      // after pushed a downstream event (from audio thread),
      // request host to call on_main_thread() for polling events
      if (this->host && this->host->request_callback) {
        bool expected = false;
        if (mainThreadRequestedFlag.compare_exchange_strong(
                expected, true, std::memory_order_acq_rel)) {
          this->host->request_callback(this->host);
        }
      }
    });
  }

  void clearMainThreadRequestedFlag() {
    mainThreadRequestedFlag.store(false, std::memory_order_release);
  }

public:
  EntryController(SynthesizerBase &synth)
      : synth(&synth), processorAdapter(*this->synth, eventBridge) {}

  void initialize() override {
    uint64_t maxAddress = 0xFFFFFFFE; // for valid clap_id
    parameterManager.setupParameters(*synth, maxAddress);
    setupEventBridgeCallbacks();
  }

  void terminate() override {}

  ~EntryController() override {
    eventBridge.clearDownstreamEventPushCallback();
    guiDestroy();
  }

  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) override {
    processorAdapter.prepareProcessing(sampleRate, maxFrameCount);
  }

  clap_process_status process(const clap_process_t *process) override {
    return processorAdapter.process(process);
  }

  void flush(const clap_input_events_t *in,
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
    clearMainThreadRequestedFlag();
    DownstreamEvent e;
    while (eventBridge.popDownstreamEvent(e)) {
      if (e.type == DownstreamEventType::ParameterChange) {
        parameterManager.setParameter(e.param.paramId, e.param.value, true);
      }
      downstreamEventPort.emitDownstreamEvent(e);
    }
  }

  bool guiCreate() override {
    webView = std::make_unique<sonic_common::MacWebView>();
    std::string url = synth->getEditorPageUrl();
    webView->loadUrl(url);
    webViewBridge.onWebViewOpen(webView.get());
    return true;
  }

  void guiDestroy() override {
    if (webView) {
      webViewBridge.onWebViewClose();
      webView->setMessageReceiver(nullptr);
      webView->removeFromParent();
      webView.reset();
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

  bool guiShow() override { return true; }

  bool guiHide() override { return true; }
};