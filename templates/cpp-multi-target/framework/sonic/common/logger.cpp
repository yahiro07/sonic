#include "./logger.h"
#include "./spsc-queue.h"
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

namespace sonic {

enum class LogKind {
  Trace,
  Info,
  Log,
  Warn,
  Error,
};

struct LogItem {
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

static std::string logKindToString(LogKind logKind) {
  switch (logKind) {
  case LogKind::Trace:
    return "trace";
  case LogKind::Info:
    return "info";
  case LogKind::Log:
    return "log";
  case LogKind::Warn:
    return "warn";
  case LogKind::Error:
    return "error";
  }
}

static std::string serializeLogItem(const LogItem &item) {
  const auto escapedMessage = jsonEscapeString(item.message);

  auto timeStampStr = std::to_string(item.timestamp);
  std::string jsonStr = "{\"timestamp\": " + timeStampStr +
                        ", \"subsystem\": \"" + item.subsystem +
                        "\", \"logKind\": \"" + item.logKind +
                        "\", \"message\": \"" + escapedMessage + "\"}";
  return jsonStr;
}

static const std::unordered_map<std::string, std::string> subsystemIcons = {
    {"host", "🟣"},
    {"ext", "🔸"},
    {"ui", "🔹"},
    {"dsp", "🔺"},
};

static const std::unordered_map<std::string, std::string> logKindIcons = {
    {"trace", "🔽"},
    //
    {"info", "◻️"},
    {"log", "▫️"},
    {"warn", "⚠️"},
    {"error", "📛"},
};

// 00:00:00.000
static std::string formatTimestamp(double timestamp) {
  auto totalMs = static_cast<int64_t>(timestamp);
  auto h = totalMs / (1000 * 60 * 60);
  auto m = (totalMs % (1000 * 60 * 60)) / (1000 * 60);
  auto s = (totalMs % (1000 * 60)) / 1000;
  auto ms = totalMs % 1000;
  char buf[16];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d", h, m, s, ms);
  return std::string(buf);
}

static void printLogInternal(const LogItem &item) {
  auto ts = formatTimestamp(item.timestamp);
  auto ssIcon = subsystemIcons.at(item.subsystem);
  auto kindIcon = logKindIcons.at(item.logKind);
  printf("%s [%s %s] %s %s\n", ts.c_str(), ssIcon.c_str(),
         item.subsystem.c_str(), kindIcon.c_str(), item.message.c_str());
}

static double getSystemTimestamp() {
  return static_cast<double>(
             std::chrono::system_clock::now().time_since_epoch().count()) /
         1000;
}

// audio thread safe logger implementation
// show logs in terminal
// send logs to local udp logger server at 127.0.0.1:9001
class Logger::LoggerImpl {
private:
  std::string subsystem;
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
  LoggerImpl(std::string subsystem) : subsystem(subsystem) {}
  ~LoggerImpl() noexcept {
    try {
      stop();
    } catch (...) {
      // best-effort: destructors must not throw
    }
  }

  std::string &getSubsystem() { return subsystem; }

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
          logRaw(logKindStr.c_str(), subsystem.c_str(), msg.timestamp,
                 msg.data);
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
    if (sock >= 0) {
      close(sock);
      sock = -1;
    }
  }

  void logRaw(const char *logKind, const char *subsystem, double timestamp,
              const char *message) {
    if (running.load(std::memory_order_relaxed) == false) {
      return;
    }
    auto logItem = LogItem{
        .timestamp = timestamp,
        .subsystem = subsystem,
        .logKind = logKind,
        .message = message,
    };
    auto jsonStr = serializeLogItem(logItem);

    sendto(sock, jsonStr.c_str(), jsonStr.size(), 0, (struct sockaddr *)&addr,
           sizeof(addr));
    printLogInternal(logItem);
  }

  void logVA(LogKind logKind, const char *fmt, va_list args) {
    if (running.load(std::memory_order_relaxed) == false) {
      return;
    }
    Message msg{
        .logKind = logKind,
        .timestamp = getSystemTimestamp(),
    };
    vsnprintf(msg.data, sizeof(msg.data), fmt, args);
    queue.push(msg);
  }
};

#if !defined(NDEBUG)
Logger::Logger(std::string subsystem)
    : impl(std::make_unique<LoggerImpl>(subsystem)) {}

Logger::~Logger() { stop(); }

void Logger::start() { impl->start(); }

void Logger::stop() { impl->stop(); }

void Logger::trace(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  impl->logVA(LogKind::Trace, fmt, args);
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
  auto subsystem = impl->getSubsystem();
  impl->logRaw("log", subsystem.c_str(), getSystemTimestamp(), message);
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

void Logger::trace(const char *fmt, ...) {}

void Logger::info(const char *fmt, ...) {}

void Logger::log(const char *fmt, ...) {}

void Logger::warn(const char *fmt, ...) {}

void Logger::error(const char *fmt, ...) {}

void Logger::directLogNonRT(const char *message) {}

void Logger::forwardUiLog(const char *logKind, double timestamp,
                          const char *message) {}

#endif

Logger logger("ext");

} // namespace sonic