#include "udp-log-emitter.h"
#include "sonic/common/logger.h"
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace sonic {

class UdpLogEmitter::UdpLogEmitterImpl {
private:
  int port;
  int sock = -1;
  struct sockaddr_in addr{};

public:
  UdpLogEmitterImpl(int port) : port(port) {}
  ~UdpLogEmitterImpl() noexcept {
    try {
      if (sock >= 0) {
        close(sock);
        sock = -1;
      }
    } catch (...) {
    }
  }

  void emit(std::string &jsonStr) {
    if (sock < 0) {
      sock = socket(AF_INET, SOCK_DGRAM, 0);
      if (sock < 0) {
        return;
      }
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    }
    sendto(sock, jsonStr.c_str(), jsonStr.size(), 0, (struct sockaddr *)&addr,
           sizeof(addr));
  }

  void stop() {}
};

UdpLogEmitter::UdpLogEmitter(int port)
    : impl(std::make_unique<UdpLogEmitterImpl>(port)) {}

UdpLogEmitter::~UdpLogEmitter() {}

void UdpLogEmitter::emit(std::string &jsonStr) { impl->emit(jsonStr); }

LogEmitter *createUdpLogEmitter() { return new UdpLogEmitter(); }

} // namespace sonic
