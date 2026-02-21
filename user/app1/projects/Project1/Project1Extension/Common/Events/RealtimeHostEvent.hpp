#pragma once 

enum class RealtimeHostEventType {
  None = 0,
  NoteOn,
  NoteOff,
  Tempo,
  PlayState
};

struct RealtimeHostEvent {
  RealtimeHostEventType type;
  int data1;   // noteNumber, playState(0 or 1)
  float data2; // velocity(0.0~1.0), tempo
};
