import CoreAudioKit
import os

public class AudioUnitFactory: NSObject, AUAudioUnitFactory {
  var auAudioUnit: AUAudioUnit?

  public func beginRequest(with context: NSExtensionContext) {}

  @objc
  public func createAudioUnit(with componentDescription: AudioComponentDescription) throws
    -> AUAudioUnit
  {
    auAudioUnit = try WrapperAuv3AudioUnit(componentDescription: componentDescription, options: [])
    guard let audioUnit = auAudioUnit else {
      fatalError("Failed to create Project1Extension")
    }
    return audioUnit
  }

}
