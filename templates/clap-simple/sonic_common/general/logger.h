#pragma once
#include <memory>

namespace sonic_common {

class Logger {
public:
  Logger();
  ~Logger();
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
  void start();
  void stop();
  void mark(const char *fmt, ...);
  void info(const char *fmt, ...);
  void log(const char *fmt, ...);
  void warn(const char *fmt, ...);
  void error(const char *fmt, ...);
  void directLogNonRT(const char *message);
  void forwardUiLog(const char *logKind, double timestamp, const char *message);

private:
  class LoggerImpl;
  std::unique_ptr<LoggerImpl> impl;
};

extern Logger logger;

} // namespace sonic_common