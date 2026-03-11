#pragma once
#include <functional>

namespace sonic {

template <typename... Args> class SingleListenerPort {
private:
  std::function<void(Args...)> listener;

public:
  void subscribe(std::function<void(Args...)> listener) {
    this->listener = listener;
  }
  void unsubscribe() { this->listener = nullptr; }

  void call(Args... args) {
    if (listener) {
      listener(args...);
    }
  }
};

} // namespace sonic