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

    private var observation: NSKeyValueObservation?

    public func beginRequest(with context: NSExtensionContext) {

    }

    @objc
    public func createAudioUnit(with componentDescription: AudioComponentDescription) throws -> AUAudioUnit {
//        auAudioUnit = try Project1ExtensionAudioUnit(componentDescription: componentDescription, options: [])

        auAudioUnit = try WrapperAuv3AudioUnit(componentDescription: componentDescription, options: [])
        guard let audioUnit = auAudioUnit else {
            fatalError("Failed to create Project1Extension")
        }

//        audioUnit.setupParameterTree(Project1ExtensionParameterSpecs.createAUParameterTree())
//
//        self.observation = audioUnit.observe(\.allParameterValues, options: [.new]) { object, change in
//            guard let tree = audioUnit.parameterTree else { return }
//            
//            // This insures the Audio Unit gets initial values from the host.
//            for param in tree.allParameters { param.value = param.value }
//        }

        return audioUnit
    }
    
}
