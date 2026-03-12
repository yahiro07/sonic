#pragma once
#include "../../../common/spsc-queue.h"

namespace sonic_vst {

using namespace sonic;

enum class RealtimeHostEventType {
  None = 0,
  NoteOn,
  NoteOff,
  Tempo,
  PlayState
};

struct RealtimeHostEvent {
  RealtimeHostEventType type;
  int data1;    // noteNumber, playState(0 or 1)
  double data2; // velocity(0.0~1.0), tempo
};

class RealtimeHostEventQueue {
private:
  SPSCQueue<RealtimeHostEvent, 256> queue;

public:
  bool push(const RealtimeHostEvent &e) { return queue.push(e); }
  bool pop(RealtimeHostEvent &e) { return queue.pop(e); }
};

} // namespace sonic_vst