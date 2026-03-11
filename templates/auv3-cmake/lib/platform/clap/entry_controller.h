#pragma once

// #include "../portable/downstream_event_port.h"
// #include "../portable/parameter_manager.h"
// #include "../portable/upstream_event_port.h"
// #include "./clap_data_helper.h"
#include "./entry_controller_interface.h"
// #include "./processor_adapter.h"
// #include "clap/process.h"
// #include "my_clap_1/portable/event_bridge.h"
// #include "my_clap_1/portable/interfaces.h"
// #include "my_clap_1/portable/telemetry_support.h"
// #include "my_clap_1/portable/webview_bridge.h"
#include "../../common/mac-web-view.h"
// #include "sonic_common/logic/parameter_definitions_provider.h"
// #include <CoreFoundation/CFNotificationCenter.h>
// #include <atomic>
#include <cassert>
// #include <cstddef>
// #include <cstring>
#include <memory>
// #include <string>
#include "../../api/synthesizer-base.h"

#define GUI_API CLAP_WINDOW_API_COCOA

using IPluginSynthesizer = sonic::SynthesizerBase;

class EntryController : public IEntryController {
private:
  std::unique_ptr<IPluginSynthesizer> synth;
  // Eventbridge eventBridge;
  // ProcessorAdapter processorAdapter{*synth, eventBridge};
  // TelemetrySupport telemetrySupport{*synth, processorAdapter};

  std::unique_ptr<sonic::MacWebView> webView;

  // sonic_common::ParameterDefinitionsProvider parameterDefinitionsProvider;

  // ParameterManager parameterManager{parameterDefinitionsProvider};
  // UpstreamEventPort upstreamEventPort{parameterDefinitionsProvider,
  //                                     parameterManager, eventBridge};
  // DownstreamEventPort downstreamEventPort;
  // WebViewBridge webViewBridge{parameterManager, upstreamEventPort,
  //                             downstreamEventPort, telemetrySupport};

  // std::atomic<bool> mainThreadRequestedFlag = false;

  // clap_id telemetryUpdateTimerId = -1;
  // int telemetryTimerRegisteredIntervalMs = 0;

  // void setupEventBridgeCallbacks() {
  //   eventBridge.setUpstreamEventPushCallback([this]() {
  //     // after pushed an upstream event (from main thread),
  //     // request host to call flush() for emitting events
  //     if (this->hostParams) {
  //       this->hostParams->request_flush(this->host);
  //     }
  //   });
  //   eventBridge.setDownstreamEventPushCallback([this]() {
  //     // after pushed a downstream event (from audio thread),
  //     // request host to call on_main_thread() for polling events
  //     if (this->host && this->host->request_callback) {
  //       bool expected = false;
  //       if (mainThreadRequestedFlag.compare_exchange_strong(
  //               expected, true, std::memory_order_acq_rel)) {
  //         this->host->request_callback(this->host);
  //       }
  //     }
  //   });
  // }

  // void clearMainThreadRequestedFlag() {
  //   mainThreadRequestedFlag.store(false, std::memory_order_release);
  // }

  // void setupTelemetryUpdateTimer(bool enabled, int intervalMs) {
  //   if (!(hostTimerSupport && hostTimerSupport->register_timer &&
  //         hostTimerSupport->unregister_timer))
  //     return;
  //   if (enabled) {
  //     if (telemetryUpdateTimerId != -1) {
  //       if (telemetryTimerRegisteredIntervalMs == intervalMs) {
  //         // already registered with the same interval, no need to
  //         re-register return;
  //       }
  //       hostTimerSupport->unregister_timer(host, telemetryUpdateTimerId);
  //     }
  //     hostTimerSupport->register_timer(host, intervalMs,
  //                                      &telemetryUpdateTimerId);
  //     telemetryTimerRegisteredIntervalMs = intervalMs;
  //     printf("registered timer id: %d\n", telemetryUpdateTimerId);
  //   } else {
  //     hostTimerSupport->unregister_timer(host, telemetryUpdateTimerId);
  //     telemetryUpdateTimerId = -1;
  //     telemetryTimerRegisteredIntervalMs = 0;
  //   }
  // }

public:
  EntryController(IPluginSynthesizer &synth) : synth(&synth) {}

  void initialize() override {
    // auto thereadId = std::this_thread::get_id();
    // printf("initialize thread id: %d\n",
    //        std::hash<std::thread::id>{}(thereadId));

    // uint64_t maxAddress = 0xFFFFFFFE; // for valid clap_id
    // parameterManager.setupParameters(*synth, maxAddress);
    // setupEventBridgeCallbacks();
    // telemetrySupport.setup();
    // telemetrySupport.setTelemetryTimerRequestCallback(
    //     [this](bool enabled, int intervalMs) {
    //       this->setupTelemetryUpdateTimer(enabled, intervalMs);
    //     });
  }

  ~EntryController() override {
    // setupTelemetryUpdateTimer(false, 0);
    // telemetrySupport.setTelemetryTimerRequestCallback(nullptr);
    // eventBridge.clearUpstreamEventPushCallback();
    // eventBridge.clearDownstreamEventPushCallback();
    guiDestroy();
  }

  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) override {
    // processorAdapter.prepareProcessing(sampleRate, maxFrameCount);
  }

  // bool first = true;

  clap_process_status process(const clap_process_t *process) override {
    // if (first) {
    //   auto thereadId = std::this_thread::get_id();
    //   printf("audio thread thread id: %d\n",
    //          std::hash<std::thread::id>{}(thereadId));
    //   first = false;
    // }
    // auto res = processorAdapter.process(process);
    // if (res == CLAP_PROCESS_CONTINUE) {
    //   telemetrySupport.process();
    // }
    // return res;
    return CLAP_PROCESS_CONTINUE;
  }

  void flush(const clap_input_events_t *in,
             const clap_output_events_t *out) override {
    // processorAdapter.flushParameters(in, out);
  }

  uint32_t getParameterCount() const override {
    // return parameterDefinitionsProvider.getParameterItems().size();
    return 0;
  }

  void getParameterInfo(uint32_t index,
                        clap_param_info_t *info) const override {
    // auto parameterItems = parameterDefinitionsProvider.getParameterItems();
    // auto &item = parameterItems[index];
    // assignParameterInfo(info, item);
  }

  double getParameterValue(clap_id id) const override {
    // return parameterManager.getParameter(id);
    return 0;
  }

  void onTimer(clap_id timerId) override {
    // printf("onTimer called, timerId: %d\n", timerId);
    // if (timerId == telemetryUpdateTimerId) {
    //   telemetrySupport.updateTelemetries();
    // }
  }

  void onMainThread() override {
    // auto thereadId = std::this_thread::get_id();
    // printf("main thread thread id: %d\n",
    //        std::hash<std::thread::id>{}(thereadId));

    // clearMainThreadRequestedFlag();
    // DownstreamEvent e;
    // while (eventBridge.popDownstreamEvent(e)) {
    //   if (e.type == DownstreamEventType::ParameterChange) {
    //     parameterManager.setParameter(e.param.paramId, e.param.value, true);
    //   }
    //   downstreamEventPort.emitDownstreamEvent(e);
    // }
  }

  bool guiCreate() override {
    webView = std::make_unique<sonic::MacWebView>();
    std::string url = synth->getEditorPageUrl();
    webView->loadUrl(url);
    // webViewBridge.onWebViewOpen(webView.get());
    return true;
  }

  void guiDestroy() override {
    if (webView) {
      // webViewBridge.onWebViewClose();
      webView->setMessageReceiver(nullptr);
      webView->removeFromParent();
      webView.reset();
    }
  }

  bool guiSetParent(const clap_window_t *window) override {
    if (!webView) {
      return false;
    }
    assert(0 == std::strcmp(window->api, GUI_API));
    webView->attachToParent(window->cocoa);
    return true;
  }

  bool guiSetSize(uint32_t width, uint32_t height) override {
    if (!webView) {
      return false;
    }
    webView->setFrame(0, 0, width, height);
    return true;
  }

  bool guiShow() override { return true; }

  bool guiHide() override { return true; }
};