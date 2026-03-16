#pragma once

#include "../logic/interfaces.h"
#include <atomic>
#include <functional>
#include <thread>

namespace vst_support {

class MainLoopTimer : public sonic::IMainLoopTimer {
private:
  std::function<void()> callback = nullptr;
  int timerId = 0;
  std::atomic<bool> running{false};
  std::thread th;

public:
  void setCallback(std::function<void()> callback) override {
    this->callback = callback;
  }
  void clearCallback() override { this->callback = nullptr; }
  void start() override {
    running = true;
    th = std::thread([this] {
      while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        callback();
      }
    });
  }
  void stop() override {
    running = false;
    if (th.joinable())
      th.join();
  }
};
} // namespace vst_support
