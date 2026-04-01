#pragma once
#include <memory>

namespace clap_dev_host {

class Logger {
public:
  Logger(std::string subsystem);
  ~Logger();
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
  void start();
  void stop();
  void trace(const char *fmt, ...);
  void info(const char *fmt, ...);
  void log(const char *fmt, ...);
  void warn(const char *fmt, ...);
  void error(const char *fmt, ...);
  void directLogNonRT(const char *message);

private:
  class LoggerImpl;
  std::unique_ptr<LoggerImpl> impl;
};

extern Logger logger;

} // namespace clap_dev_host