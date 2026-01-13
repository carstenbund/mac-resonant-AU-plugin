//
//  AudioUnitHostModel.swift
//  ModalAttractors
//
//  Created by Carsten on 1/7/26.
//

import SwiftUI
import CoreMIDI
import AudioToolbox
import os

@MainActor
@Observable
class AudioUnitHostModel {
    /// The playback engine used to play audio.
    private let playEngine = SimplePlayEngine()

    /// The model providing information about the current Audio Unit
    var viewModel = AudioUnitViewModel()

    var isPlaying: Bool { playEngine.isPlaying }

    /// Audio Component Description
    let type: String
    let subType: String
    let manufacturer: String

    let wantsAudio: Bool
    let wantsMIDI: Bool
    let isFreeRunning: Bool

    let auValString: String

    init(type: String = "aumu", subType: String = "Test", manufacturer: String = "Bund") {
        self.type = type
        self.subType = subType
        self.manufacturer = manufacturer
        let wantsAudio = type.fourCharCode == kAudioUnitType_MusicEffect || type.fourCharCode == kAudioUnitType_Effect
        self.wantsAudio = wantsAudio

        let wantsMIDI = type.fourCharCode == kAudioUnitType_MIDIProcessor ||
        type.fourCharCode == kAudioUnitType_MusicDevice ||
        type.fourCharCode == kAudioUnitType_MusicEffect
        self.wantsMIDI = wantsMIDI

        let isFreeRunning = type.fourCharCode == kAudioUnitType_MIDIProcessor ||
        type.fourCharCode == kAudioUnitType_MusicDevice ||
        type.fourCharCode == kAudioUnitType_Generator
        self.isFreeRunning = isFreeRunning

        auValString = "\(type) \(subType) \(manufacturer)"

        loadAudioUnit()
    }

    private func loadAudioUnit() {
		Task {
            AULogger.host.info("loading audio unit type=\(self.type, privacy: .public) subType=\(self.subType, privacy: .public) manufacturer=\(self.manufacturer, privacy: .public)")
			let viewController = await playEngine.initComponent(type: type, subType: subType, manufacturer: manufacturer)

            if viewController == nil {
                AULogger.host.error("Failed to load Audio Unit - component not found or failed to instantiate")
                self.viewModel = AudioUnitViewModel(showAudioControls: self.wantsAudio,
                                                    showMIDIContols: self.wantsMIDI,
                                                    title: self.auValString,
                                                    message: "Failed to load (\(self.auValString)). The plugin may need to be registered. Please restart the app.",
                                                    viewController: nil)
            } else {
                AULogger.host.info("audio unit loaded successfully. viewController=\(String(describing: viewController), privacy: .public)")
                self.viewModel = AudioUnitViewModel(showAudioControls: self.wantsAudio,
                                                    showMIDIContols: self.wantsMIDI,
                                                    title: self.auValString,
                                                    message: "Successfully loaded (\(self.auValString))",
                                                    viewController: viewController)

                if self.isFreeRunning {
                    self.playEngine.startPlaying()
                }
            }
		}
    }

    func startPlaying() {
        playEngine.startPlaying()
    }

    func stopPlaying() {
        playEngine.stopPlaying()
    }
}
