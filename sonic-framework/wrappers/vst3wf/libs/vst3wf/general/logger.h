#pragma once
#include <memory>

namespace vst3wf {

class Logger {
private:
  class LoggerImpl;
  std::unique_ptr<LoggerImpl> impl;

public:
  Logger();
  ~Logger();
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
  void start();
  void stop();
  void log(const char *fmt, ...);
  void directLogNonRT(const char *message);
};

extern Logger logger;

} // namespace vst3wf