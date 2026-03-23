#include <functional>
#include <map>

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

template <typename... Args> class MultipleListenerPort {
private:
  std::map<int, std::function<void(Args...)>> listeners;
  int nextToken = 0;

public:
  int subscribe(std::function<void(Args...)> listener) {
    int token = nextToken++;
    listeners[token] = listener;
    return token;
  }
  void unsubscribe(int token) { listeners.erase(token); }

  void call(Args... args) {
    for (auto &pair : listeners) {
      pair.second(args...);
    }
  }
};

} // namespace sonic