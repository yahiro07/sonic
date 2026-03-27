import Combine

//An interface for accessing AudioUnit functionality from the UI
//・Note-on/note-off events from the UI keyboard
//・Subscription to events sent by the host
//Provides these functions
protocol AudioUnitPresenter {
  var isHostedInStandaloneApp: Bool { get }
  func noteOnFromUI(_ noteNumber: Int, velocity: Float)
  func noteOffFromUI(_ noteNumber: Int)
  var events: AnyPublisher<DownstreamEvent, Never> { get }
  func applyParametersState(
    _ parametersVersion: Int, _ parameters: [String: Float]
  )
}

final class AudioUnitPresenterImpl: AudioUnitPresenter {
  private var audioUnit: AudioUnit?

  private(set) var parametersVersion: Int = 0

  func setAudioUnit(_ audioUnit: AudioUnit) {
    self.audioUnit = audioUnit
  }

  var isHostedInStandaloneApp: Bool {
    return audioUnit?.isHostedInStandaloneApp ?? false
  }

  func noteOnFromUI(_ noteNumber: Int, velocity: Float) {
    let byteVelocity = UInt8(max(0, min(127, Int(velocity * 127))))
    audioUnit?.pushScheduledMidiEvent([0x90, UInt8(noteNumber), byteVelocity])
  }

  func noteOffFromUI(_ noteNumber: Int) {
    audioUnit?.pushScheduledMidiEvent([0x80, UInt8(noteNumber), 0])
  }

  private let subject = PassthroughSubject<DownstreamEvent, Never>()

  var events: AnyPublisher<DownstreamEvent, Never> {
    subject.eraseToAnyPublisher()
  }

  func emitEvent(_ event: DownstreamEvent) {
    subject.send(event)
  }

  func applyParametersState(
    _ parametersVersion: Int, _ parameters: [String: Float]
  ) {
    audioUnit?.applyParametersState(parametersVersion, parameters)
  }

  // Use a timer on the main thread to poll and send events
  // Send MIDI note events captured on the audio thread to the UI on the main thread
  // The AudioUnitViewController is expected to run a loop at regular intervals to call this method
  func drainEventsOnMainThread(maxCount: Int = 64) {
    var count = 0
    while count < maxCount, let event = audioUnit?.pullRTAudioPortalEventOne() {
      self.emitEvent(event)
      count += 1
    }
    if audioUnit?.currentPresetParametersVersion != parametersVersion {
      parametersVersion = audioUnit?.currentPresetParametersVersion ?? 0
      self.emitEvent(.parametersVersionChanged(parametersVersion))
    }
  }
}
