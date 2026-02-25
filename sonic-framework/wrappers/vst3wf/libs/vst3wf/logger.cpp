#include "./logger.h"
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <memory>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace vst3wf {

template <typename T, size_t Capacity>

class SPSCQueue {
  static_assert(Capacity >= 2, "Capacity must be >= 2");
  static_assert((Capacity & (Capacity - 1)) == 0,
                "Capacity must be power of two");

private:
  std::atomic<uint32_t> readIndex = 0;
  std::atomic<uint32_t> writeIndex = 0;
  T buffer[Capacity];

public:
  SPSCQueue() = default;
  ~SPSCQueue() = default;

  bool push(const T &item) noexcept {
    uint32_t currentWrite = writeIndex.load(std::memory_order_relaxed);
    uint32_t nextWrite = (currentWrite + 1) & (Capacity - 1);
    if (nextWrite == readIndex.load(std::memory_order_acquire)) {
      // Queue is full
      return false;
    }
    buffer[currentWrite] = item;
    writeIndex.store(nextWrite, std::memory_order_release);
    return true;
  }

  bool pop(T &item) noexcept {
    uint32_t currentRead = readIndex.load(std::memory_order_relaxed);
    if (currentRead == writeIndex.load(std::memory_order_acquire)) {
      // Queue is empty
      return false;
    }
    item = buffer[currentRead];
    readIndex.store((currentRead + 1) & (Capacity - 1),
                    std::memory_order_release);
    return true;
  }
};

class Logger::LoggerImpl {
private:
  std::thread worker;
  std::atomic<bool> running{false};
  int sock = -1;
  struct sockaddr_in addr;
  struct Message {
    char data[256];
  };
  SPSCQueue<Message, 32> queue;

public:
  void start() {
    if (running.exchange(true))
      return;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9001);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    worker = std::thread([this]() {
      while (running.load(std::memory_order_relaxed)) {
        Message msg;
        if (queue.pop(msg)) {
          logRaw(msg.data);
        } else {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      }
    });
  }
  void stop() {
    running = false;
    if (worker.joinable()) {
      worker.join();
    }
    close(sock);
  }

  void logRaw(const char *message) {
    sendto(sock, message, strlen(message), 0, (struct sockaddr *)&addr,
           sizeof(addr));
    printf("%s\n", message);
  }

  void logVA(const char *fmt, va_list args) {
    Message msg{};
    vsnprintf(msg.data, sizeof(msg.data), fmt, args);
    queue.push(msg);
  }
};

#if !defined(NDEBUG)
Logger::Logger() : impl(std::make_unique<LoggerImpl>()) {}

Logger::~Logger() = default;

void Logger::start() { impl->start(); }

void Logger::stop() { impl->stop(); }

void Logger::log(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  impl->logVA(fmt, args);
  va_end(args);
}

void Logger::directLogNonRT(const char *message) { impl->logRaw(message); }

#else

Logger::Logger() {}

Logger::~Logger() {}

void Logger::start() {}

void Logger::stop() {}

void Logger::log(const char *fmt, ...) {}

void Logger::directLogNonRT(const char *message) {}

#endif

Logger logger;

} // namespace vst3wf