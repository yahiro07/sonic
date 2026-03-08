//
//  AudioUnitFactory.swift
//  Project1Extension
//
//  Created by ore on 2026/03/08.
//

import CoreAudioKit
import os

private let log = Logger(subsystem: "com.bundle.id.Project1Extension", category: "AudioUnitFactory")

public class AudioUnitFactory: NSObject, AUAudioUnitFactory {
    var auAudioUnit: AUAudioUnit?

    public func beginRequest(with context: NSExtensionContext) {}

    @objc
    public func createAudioUnit(with componentDescription: AudioComponentDescription) throws -> AUAudioUnit {
        auAudioUnit = try WrapperAuv3AudioUnit(componentDescription: componentDescription, options: [])
        guard let audioUnit = auAudioUnit else {
            fatalError("Failed to create Project1Extension")
        }
        return audioUnit
    }
    
}
