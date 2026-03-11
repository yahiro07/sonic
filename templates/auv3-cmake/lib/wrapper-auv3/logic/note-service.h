#pragma once
#include "../common/listener-port.h"
namespace sonic {

class NoteService {
public:
  // note request: DSP <-- Controller <-- UI
  SingleListenerPort<int, float> noteRequestPort;

  // active note state: Host,DSP --> Controller --> UI
  MultipleListenerPort<int, float> hostNotePort;
};
} // namespace sonic
