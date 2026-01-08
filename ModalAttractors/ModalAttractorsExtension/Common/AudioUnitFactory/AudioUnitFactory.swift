//
//  AudioUnitFactory.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/7/26.
//

import CoreAudioKit
import os

private let log = Logger(subsystem: "com.bundle.id.ModalAttractorsExtension", category: "AudioUnitFactory")

public class AudioUnitFactory: NSObject, AUAudioUnitFactory {
    var auAudioUnit: AUAudioUnit?

    private var observation: NSKeyValueObservation?

    public func beginRequest(with context: NSExtensionContext) {
        // Extension request handling (if needed)
    }

    @objc
    public func createAudioUnit(with componentDescription: AudioComponentDescription) throws -> AUAudioUnit {
        auAudioUnit = try ModalAttractorsExtensionAudioUnit(
            componentDescription: componentDescription,
            options: []
        )

        guard let audioUnit = auAudioUnit else {
            throw NSError(
                domain: NSOSStatusErrorDomain,
                code: Int(kAudioUnitErr_FailedInitialization),
                userInfo: [NSLocalizedDescriptionKey: "Failed to create ModalAttractorsExtension"]
            )
        }

        // Observe allParameterValues to ensure host can set initial values
        self.observation = audioUnit.observe(\.allParameterValues, options: [.new]) { object, change in
            guard let tree = audioUnit.parameterTree else { return }

            // This ensures the Audio Unit gets initial values from the host
            for param in tree.allParameters {
                param.value = param.value
            }
        }

        return audioUnit
    }
}
