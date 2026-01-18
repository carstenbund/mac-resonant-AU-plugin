//
//  ModalAttractorsAUViewControllerExtension.swift
//  ModalAttractorsFramework
//
//  ModalAttractorsAUViewController is the app extension's principal class,
//  responsible for creating both the audio unit and its view.
//

import CoreAudioKit
import os

private let log = Logger(subsystem: "com.bund.media.ModalAttractorsExtension", category: "Factory")

extension ModalAttractorsAUViewController: AUAudioUnitFactory {

    /// Creates the audio unit when requested by the host
    /// This is called by the system when the AU is instantiated
    @objc
    public func createAudioUnit(with componentDescription: AudioComponentDescription) throws -> AUAudioUnit {
        log.info("Creating audio unit...")

        let au = try ModalAttractorsExtensionAudioUnit(
            componentDescription: componentDescription,
            options: []
        )

        // Observe allParameterValues to ensure host can set initial values
        observation = au.observe(\.allParameterValues, options: [.new]) { [weak au] _, _ in
            guard let audioUnit = au, let tree = audioUnit.parameterTree else { return }
            // This ensures the Audio Unit gets initial values from the host
            for param in tree.allParameters {
                param.value = param.value
            }
        }

        log.info("Audio unit created successfully")

        // Set the audio unit - this triggers binding via didSet
        DispatchQueue.main.async { [weak self] in
            self?.audioUnit = au
        }

        return au
    }
}
