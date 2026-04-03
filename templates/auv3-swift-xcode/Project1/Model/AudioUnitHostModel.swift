import AVFAudio
import AudioToolbox
import CoreMIDI
import SwiftUI

@MainActor
@Observable
class AudioUnitHostModel {
  /// The playback engine used to play audio.
  private let playEngine = SimplePlayEngine()

  /// The model providing information about the current Audio Unit
  var viewModel = AudioUnitViewModel()

  var isPlaying: Bool { playEngine.isPlaying }

  var audioUnitCrashed = false

  /// Audio Component Description
  let type: String
  let subType: String
  let manufacturer: String

  let wantsAudio: Bool
  let wantsMIDI: Bool
  let isFreeRunning: Bool

  let auValString: String

  private let instanceInvalidationNotifcation = Notification.Name(
    String(kAudioComponentInstanceInvalidationNotification))

  var validationResult: AudioComponentValidationResult?
  var currentValidationData: String?

  init(type: String = "aumu", subType: String = "prj2", manufacturer: String = "Myco") {
    logger.trace("------------------------------------------------")
    logger.trace("AudioUnitHostModel init")

    self.type = type
    self.subType = subType
    self.manufacturer = manufacturer
    let wantsAudio =
      type.fourCharCode == kAudioUnitType_MusicEffect || type.fourCharCode == kAudioUnitType_Effect
    self.wantsAudio = wantsAudio

    let wantsMIDI =
      type.fourCharCode == kAudioUnitType_MIDIProcessor
      || type.fourCharCode == kAudioUnitType_MusicDevice
      || type.fourCharCode == kAudioUnitType_MusicEffect
    self.wantsMIDI = wantsMIDI

    let isFreeRunning =
      type.fourCharCode == kAudioUnitType_MIDIProcessor
      || type.fourCharCode == kAudioUnitType_MusicDevice
      || type.fourCharCode == kAudioUnitType_Generator
    self.isFreeRunning = isFreeRunning

    auValString = "\(type) \(subType) \(manufacturer)"

    setupNotifications()
    loadAudioUnit()
  }

  private func loadAudioUnit() {
    Task {
      let viewController = await playEngine.initComponent(
        type: type, subType: subType, manufacturer: manufacturer)

      self.restoreState()

      if false {
        //do validation
        if let audioUnit = playEngine.avAudioUnit {
          Task { @MainActor in
            logger.log("--Start validation--")
            let (validationResult, validationData) = await validateAU(audioUnit: audioUnit)
            self.validationResult = validationResult
            self.currentValidationData = validationData
            logger.log("--End validation--")
          }
        }
      }
      self.viewModel = AudioUnitViewModel(
        showAudioControls: self.wantsAudio,
        showMIDIContols: self.wantsMIDI,
        title: self.auValString,
        message: "Successfully loaded (\(self.auValString))",
        viewController: viewController)

      if self.isFreeRunning {
        self.playEngine.startPlaying()
      }
    }
  }

  private func setupNotifications() {
    let notificationName = Notification.Name(instanceInvalidationNotifcation.rawValue)
    NotificationCenter.default.addObserver(forName: notificationName, object: nil, queue: nil) {
      [weak self] notification in
      guard let self = self else { return }
      if notification.object as? AUAudioUnit != nil {
        Task { @MainActor in
          self.audioUnitCrashed = true
        }
      }
    }
  }

  private func validateAU(audioUnit: AVAudioUnit) async -> (AudioComponentValidationResult, String)
  {
    await withCheckedContinuation { continuation in
      let validationParameters: [String: Any] = ["ForceValidation": true]

      AudioComponentValidateWithResults(
        audioUnit.auAudioUnit.component, validationParameters as CFDictionary
      ) { @Sendable result, output in
        let formattedOutput: String

        if let validationDict = output as? [String: Any],
          let rawOutput = validationDict["Output"] as? [String]
        {
          formattedOutput = rawOutput.joined(separator: "\n")
        } else {
          formattedOutput = "Validation probably crashed"
        }

        logger.log(formattedOutput)

        continuation.resume(returning: (result, formattedOutput))
      }
    }
  }

  deinit {
    NotificationCenter.default.removeObserver(
      self, name: instanceInvalidationNotifcation, object: nil)
  }

  func startPlaying() {
    playEngine.startPlaying()
  }

  func stopPlaying() {
    playEngine.stopPlaying()
  }

  func saveState() {
    guard let au = playEngine.avAudioUnit?.auAudioUnit else { return }
    let state = au.fullState
    UserDefaults.standard.set(state, forKey: "SavedAUState")
    let byteSize = calculateStateByteSize(of: state ?? [:])
    logger.log("saved state: \(byteSize)bytes")
  }

  func restoreState() {
    guard let au = playEngine.avAudioUnit?.auAudioUnit else { return }
    var state = UserDefaults.standard.dictionary(forKey: "SavedAUState") ?? au.fullState ?? [:]
    let byteSize = calculateStateByteSize(of: state)
    logger.log("restore state: \(byteSize)bytes")
    //set a flag to let the AU know it's being hosted in a standalone app
    state["MySynth1.hostedInStandaloneApp"] = true
    au.fullState = state
  }
}

private func calculateStateByteSize(of dict: [String: Any]) -> Int {
  do {
    let data = try PropertyListSerialization.data(
      fromPropertyList: dict,
      format: .binary,
      options: 0
    )
    return data.count
  } catch {
    logger.error("Failed to calculate state size: \(error)")
    return 0
  }
}
