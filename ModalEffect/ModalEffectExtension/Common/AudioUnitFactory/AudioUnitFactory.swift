//
//  AudioUnitFactory.swift
//  ModalEffectExtension
//
//  Created by Carsten on 1/7/26.
//
//  NOTE: This file is DEPRECATED.
//  The AUAudioUnitFactory protocol is now implemented by ModalEffectAUViewController
//  which is the extension's principal class. This allows proper out-of-process UI support.
//  This file is kept for reference but is no longer used.
//

import CoreAudioKit
import os

private let log = Logger(subsystem: "com.bundle.id.ModalEffectExtension", category: "AudioUnitFactory")

/// DEPRECATED: Use ModalEffectAUViewController instead.
/// This class is no longer the principal class for the extension.
@available(*, deprecated, message: "Use ModalEffectAUViewController which is now the principal class")
public class AudioUnitFactory: NSObject, AUAudioUnitFactory {
    var auAudioUnit: AUAudioUnit?

    private var observation: NSKeyValueObservation?

    public func beginRequest(with context: NSExtensionContext) {
        // Extension request handling (if needed)
    }

    @objc
    public func createAudioUnit(with componentDescription: AudioComponentDescription) throws -> AUAudioUnit {
        auAudioUnit = try ModalEffectExtensionAudioUnit(
            componentDescription: componentDescription,
            options: []
        )

        guard let audioUnit = auAudioUnit else {
            throw NSError(
                domain: NSOSStatusErrorDomain,
                code: Int(kAudioUnitErr_FailedInitialization),
                userInfo: [NSLocalizedDescriptionKey: "Failed to create ModalEffectExtension"]
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
