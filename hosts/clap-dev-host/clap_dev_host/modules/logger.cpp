#include "./logger.h"
#include "./spsc_queue.h"
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

namespace clap_dev_host {

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

class ILogEmitter {
public:
  virtual ~ILogEmitter() = default;
  virtual void emitLogItem(LogItem &item) = 0;
};

namespace udp_log_emitter {

static std::string jsonEscapeString(const std::string &input) {
  std::string out;
  out.reserve(input.size());

  auto hexDigit = [](unsigned int v) -> char {
    return static_cast<char>(v < 10 ? ('0' + v) : ('a' + (v - 10)));
  };
  for (unsigned char c : input) {
    if (c == '\\') {
      out += "\\\\";
    } else if (c == '"') {
      out += "\\\"";
    } else if (c == '\b') {
      out += "\\b";
    } else if (c == '\f') {
      out += "\\f";
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else if (c == '\t') {
      out += "\\t";
    } else if (c < 0x20) {
      out += "\\u00";
      out += hexDigit((c >> 4) & 0xF);
      out += hexDigit(c & 0xF);
    } else {
      out += static_cast<char>(c);
    }
  }
  return out;
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

class UdpLogEmitter : public ILogEmitter {
private:
  int port;
  int sock = -1;
  struct sockaddr_in addr{};

public:
  UdpLogEmitter(int port = 9001) : port(port) {}
  ~UdpLogEmitter() {
    if (sock >= 0) {
      close(sock);
      sock = -1;
    }
  }
  void emitLogItem(LogItem &item) override {
    if (sock < 0) {
      sock = socket(AF_INET, SOCK_DGRAM, 0);
      if (sock < 0) {
        return;
      }
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    }
    auto jsonStr = serializeLogItem(item);
    sendto(sock, jsonStr.c_str(), jsonStr.size(), 0, (struct sockaddr *)&addr,
           sizeof(addr));
  }
};

} // namespace udp_log_emitter

namespace stdout_log_emitter {

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

// 00:00:00.000, from midnight of today
static std::string formatTimestamp(double timestampMsFromEpoch) {

  time_t nowSec = static_cast<time_t>(timestampMsFromEpoch / 1000.0);
  struct tm t = *localtime(&nowSec);
  t.tm_hour = 0;
  t.tm_min = 0;
  t.tm_sec = 0;
  double midnightMs = static_cast<double>(mktime(&t)) * 1000.0;

  long long totalMs = static_cast<long long>(timestampMsFromEpoch - midnightMs);
  int ms = totalMs % 1000;
  long long totalSec = totalMs / 1000;
  int sec = totalSec % 60;
  int min = (totalSec / 60) % 60;
  int hour = static_cast<int>(totalSec / 3600);
  char buf[16];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d", hour, min, sec, ms);
  return buf;
}

static void printLogInternal(const LogItem &item) {
  auto ts = formatTimestamp(item.timestamp);
  auto ssIcon = subsystemIcons.at(item.subsystem);
  auto kindIcon = logKindIcons.at(item.logKind);
  printf("%s [%s %s] %s %s\n", ts.c_str(), ssIcon.c_str(),
         item.subsystem.c_str(), kindIcon.c_str(), item.message.c_str());
}

} // namespace stdout_log_emitter

static double getSystemTimestamp() {
  return static_cast<double>(
             std::chrono::system_clock::now().time_since_epoch().count()) /
         1000;
}

// audio thread safe logger implementation
// show logs in terminal
// (optional) send logs to local udp logger server at 127.0.0.1:9001
class Logger::LoggerImpl {
private:
  udp_log_emitter::UdpLogEmitter udpLogEmitter;
  std::string subsystem;
  std::thread worker;
  std::atomic<bool> running{false};
  struct Message {
    LogKind logKind;
    double timestamp;
    char data[256];
  };
  SPSCQueue<Message, 32> queue;

public:
  LoggerImpl(std::string subsystem) : subsystem(subsystem) {}
  ~LoggerImpl() { stop(); }

  std::string &getSubsystem() { return subsystem; }

  void start() {
    if (running.exchange(true))
      return;
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
    // sleep a bit to wait logs to be sent
    std::this_thread::sleep_for(std::chrono::milliseconds(4));

    running = false;
    if (worker.joinable()) {
      try {
        worker.join();
      } catch (...) {
      }
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
#ifdef SONIC_DEBUG_USE_UDP_LOGGER
    udpLogEmitter.emitLogItem(logItem);
#endif
    stdout_log_emitter::printLogInternal(logItem);
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

#if defined(SONIC_DEBUG_LOGS)
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

#else

Logger::Logger(std::string subsystem) {}

Logger::~Logger() {}

void Logger::start() {}

void Logger::stop() {}

void Logger::trace(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void Logger::info(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void Logger::log(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void Logger::warn(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void Logger::error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void Logger::directLogNonRT(const char *message) { printf("%s\n", message); }

#endif

Logger logger("host");

} // namespace clap_dev_host