#include "./logger.h"
#include "./spsc_queue.h"
#include <algorithm>
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <memory>
#include <stdint.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace sonic_vst {

enum class LogKind {
  Mark,
  Info,
  Log,
  Warn,
  Error,
  Mute,
  Unmute,
};

struct UdpLoggerLogItem {
  double timestamp;
  std::string subsystem;
  std::string logKind;
  std::string message;
};

static std::string jsonEscapeString(const std::string &input) {
  std::string out;
  out.reserve(input.size());

  auto hexDigit = [](unsigned int v) -> char {
    return static_cast<char>(v < 10 ? ('0' + v) : ('a' + (v - 10)));
  };

  for (unsigned char c : input) {
    switch (c) {
    case '\\':
      out += "\\\\";
      break;
    case '"':
      out += "\\\"";
      break;
    case '\b':
      out += "\\b";
      break;
    case '\f':
      out += "\\f";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if (c < 0x20) {
        out += "\\u00";
        out += hexDigit((c >> 4) & 0xF);
        out += hexDigit(c & 0xF);
      } else {
        out += static_cast<char>(c);
      }
      break;
    }
  }

  return out;
}

std::string logKindToString(LogKind logKind) {
  switch (logKind) {
  case LogKind::Mark:
    return "mark";
  case LogKind::Info:
    return "info";
  case LogKind::Log:
    return "log";
  case LogKind::Warn:
    return "warn";
  case LogKind::Error:
    return "error";
  case LogKind::Mute:
    return "mute";
  case LogKind::Unmute:
    return "unmute";
  }
}

static double getSystemTimestamp() {
  return static_cast<double>(
             std::chrono::system_clock::now().time_since_epoch().count()) /
         1000;
}

class Logger::LoggerImpl {
private:
  std::thread worker;
  std::atomic<bool> running{false};
  int sock = -1;
  struct sockaddr_in addr;
  struct Message {
    LogKind logKind;
    double timestamp;
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
          auto logKindStr = logKindToString(msg.logKind);
          logRaw(logKindStr.c_str(), "ext", msg.timestamp, msg.data);
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

  void logRaw(const char *logKind, const char *subsystem, double timestamp,
              const char *message) {
    auto item = UdpLoggerLogItem{
        .timestamp = timestamp,
        .subsystem = subsystem,
        .logKind = logKind,
        .message = message,
    };

    const auto escapedMessage = jsonEscapeString(item.message);

    auto timeStampStr = std::to_string(item.timestamp);
    std::string jsonStr = "{\"timestamp\": " + timeStampStr +
                          ", \"subsystem\": \"" + item.subsystem +
                          "\", \"logKind\": \"" + item.logKind +
                          "\", \"message\": \"" + escapedMessage + "\"}";
    sendto(sock, jsonStr.c_str(), jsonStr.size(), 0, (struct sockaddr *)&addr,
           sizeof(addr));
    printf("%s\n", item.message.c_str());
  }

  void logVA(LogKind logKind, const char *fmt, va_list args) {
    Message msg{
        .logKind = logKind,
        .timestamp = getSystemTimestamp(),
    };
    vsnprintf(msg.data, sizeof(msg.data), fmt, args);
    queue.push(msg);
  }
};

#if !defined(NDEBUG)
Logger::Logger() : impl(std::make_unique<LoggerImpl>()) {}

Logger::~Logger() = default;

void Logger::start() { impl->start(); }

void Logger::stop() { impl->stop(); }

void Logger::mark(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  impl->logVA(LogKind::Mark, fmt, args);
  va_end(args);
}

void Logger::info(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  impl->logVA(LogKind::Info, fmt, args);
  va_end(args);
}
void Logger::log(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  impl->logVA(LogKind::Log, fmt, args);
  va_end(args);
}

void Logger::warn(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  impl->logVA(LogKind::Warn, fmt, args);
  va_end(args);
}

void Logger::error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  impl->logVA(LogKind::Error, fmt, args);
  va_end(args);
}

void Logger::directLogNonRT(const char *message) {
  impl->logRaw("log", "ext", getSystemTimestamp(), message);
}

void Logger::forwardUiLog(const char *logKind, double timestamp,
                          const char *message) {
  impl->logRaw(logKind, "ui", timestamp, message);
}

#else

Logger::Logger() {}

Logger::~Logger() {}

void Logger::start() {}

void Logger::stop() {}

void Logger::log(const char *fmt, ...) {}

void Logger::directLogNonRT(const char *message) {}

#endif

Logger logger;

} // namespace sonic_vst