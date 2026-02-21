enum DownstreamEvent {
  case hostNoteOn(Int, Float)
  case hostNoteOff(Int)
  case hostTempo(Int)
  case hostPlayState(Bool)
  case parametersVersionChanged(Int)
}
