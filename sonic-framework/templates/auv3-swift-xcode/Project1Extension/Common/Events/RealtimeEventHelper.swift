func mapRealtimeHostEventToDownstreamEvent(_ rawEvent: RealtimeHostEvent) -> DownstreamEvent? {
  let type = rawEvent.type
  let data1: Int = Int(rawEvent.data1)
  let data2: Float = rawEvent.data2
  switch type {
  case .None:
    return nil
  case .NoteOn:
    return DownstreamEvent.hostNoteOn(data1, data2)
  case .NoteOff:
    return DownstreamEvent.hostNoteOff(data1)
  case .Tempo:
    return DownstreamEvent.hostTempo(data1)
  case .PlayState:
    return DownstreamEvent.hostPlayState(data1 != 0)
  @unknown default:
    return nil
  }
}
