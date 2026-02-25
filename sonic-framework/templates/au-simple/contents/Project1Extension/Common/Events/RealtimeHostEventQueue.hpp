#pragma once
#include "RealtimeHostEvent.hpp"
#include "SPSCQueue.hpp"

class RealtimeHostEventQueue {
private:
  SPSCQueue<RealtimeHostEvent, 256> queue;

public:
  bool push(const RealtimeHostEvent &e) { return queue.push(e); }
  bool pop(RealtimeHostEvent &e) { return queue.pop(e); }
};