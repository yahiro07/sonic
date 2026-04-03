#include <atomic>
#include <functional>
#include <thread>

namespace sonic_common {

class PollingTimer {
private:
  std::atomic<bool> running{false};
  std::thread th;

public:
  void start(std::function<void()> pollingFn, int intervalMs) {
    running = true;
    th = std::thread([this, pollingFn, intervalMs] {
      while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        pollingFn();
      }
    });
  }
  void stop() {
    running = false;
    if (th.joinable())
      th.join();
  }
};

} // namespace sonic_common