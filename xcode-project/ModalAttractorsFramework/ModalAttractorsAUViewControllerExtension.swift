//
//  ModalAttractorsAUViewControllerExtension.swift
//  ModalAttractorsFramework
//
//  ModalAttractorsAUViewController is the app extension's principal class,
//  responsible for creating both the audio unit and its view.
//

import CoreAudioKit

extension ModalAttractorsAUViewController: AUAudioUnitFactory {
    @objc
    public func createAudioUnit(with componentDescription: AudioComponentDescription) throws -> AUAudioUnit {
        audioUnit = try ModalAttractorsExtensionAudioUnit(componentDescription: componentDescription, options: [])

        guard let audioUnit = audioUnit as? ModalAttractorsExtensionAudioUnit else {
            throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioUnitErr_FailedInitialization))
        }

        // Setup parameter tree
        audioUnit.setupParameterTree(ModalAttractorsExtensionParameterSpecs.createAUParameterTree())

        // Get the parameter tree wrapper for UI
        if let paramTree = audioUnit.parameterTree {
            let wrapper = ParameterTree(auParameterTree: paramTree)
            self.paramTreeWrapper = wrapper

            // Configure the view with the wrapper
            configure(paramTreeWrapper: wrapper)

            // Observe parameter changes to ensure host can set initial values
            self.observation = audioUnit.observe(\.allParameterValues, options: [.new]) { object, change in
                guard let tree = audioUnit.parameterTree else { return }
                // This ensures the Audio Unit gets initial values from the host
                for param in tree.allParameters { param.value = param.value }
            }
        }

        return audioUnit
    }
}
