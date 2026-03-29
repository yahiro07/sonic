#pragma once
#include "logger.h"

namespace sonic {

class UdpLogEmitter : public LogEmitter {
public:
  UdpLogEmitter(int port = 9001);
  ~UdpLogEmitter();
  void emit(std::string &jsonStr) override;

private:
  class UdpLogEmitterImpl;
  std::unique_ptr<UdpLogEmitterImpl> impl;
};

} // namespace sonic