//
//  ParameterTree.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import Foundation
import AudioToolbox
import Combine
import SwiftUI

/// Main parameter tree wrapper for SwiftUI integration
/// This bridges the AU parameter tree to SwiftUI's observable system
public class ParameterTree: ObservableObject {
    public let global: GlobalParameters
    public let mode0: ModeParameters
    public let mode1: ModeParameters
    public let mode2: ModeParameters
    public let mode3: ModeParameters
    public let excitation: ExcitationParameters
    public let voice: VoiceParameters

    private let auParameterTree: AUParameterTree

    public init(auParameterTree: AUParameterTree) {
        self.auParameterTree = auParameterTree
        self.global = GlobalParameters(auParameterTree: auParameterTree)
        self.mode0 = ModeParameters(auParameterTree: auParameterTree, groupID: "mode0")
        self.mode1 = ModeParameters(auParameterTree: auParameterTree, groupID: "mode1")
        self.mode2 = ModeParameters(auParameterTree: auParameterTree, groupID: "mode2")
        self.mode3 = ModeParameters(auParameterTree: auParameterTree, groupID: "mode3")
        self.excitation = ExcitationParameters(auParameterTree: auParameterTree)
        self.voice = VoiceParameters(auParameterTree: auParameterTree)
    }

    /// Create a mock parameter tree for SwiftUI previews
    @MainActor
    public static var preview: ParameterTree {
        let tree = ModalAttractorsExtensionParameterSpecs.createAUParameterTree()
        return ParameterTree(auParameterTree: tree)
    }
}

/// Global parameter group
public class GlobalParameters: ObservableObject {
    @Published public var masterGain: ParameterWrapper
    @Published public var couplingStrength: ParameterWrapper
    @Published public var topology: ParameterWrapper
    @Published public var nodeCount: ParameterWrapper

    init(auParameterTree: AUParameterTree) {
        // Fetch parameters from tree by full identifier (group.parameter)
        let gain = auParameterTree.parameter(withID: "global.masterGain")!
        let coupling = auParameterTree.parameter(withID: "global.couplingStrength")!
        let topo = auParameterTree.parameter(withID: "global.topology")!
        let nodes = auParameterTree.parameter(withID: "global.nodeCount")!

        self.masterGain = ParameterWrapper(parameter: gain)
        self.couplingStrength = ParameterWrapper(parameter: coupling)
        self.topology = ParameterWrapper(parameter: topo)
        self.nodeCount = ParameterWrapper(parameter: nodes)
    }
}

/// Mode parameter group (generic for mode0-3)
public class ModeParameters: ObservableObject {
    @Published public var frequency: ParameterWrapper
    @Published public var damping: ParameterWrapper
    @Published public var weight: ParameterWrapper

    public let modeNumber: String

    init(auParameterTree: AUParameterTree, groupID: String) {
        self.modeNumber = groupID.replacingOccurrences(of: "mode", with: "")

        let freq = auParameterTree.parameter(withID: "\(groupID).\(groupID)Frequency")!
        let damp = auParameterTree.parameter(withID: "\(groupID).\(groupID)Damping")!
        let wt = auParameterTree.parameter(withID: "\(groupID).\(groupID)Weight")!

        self.frequency = ParameterWrapper(parameter: freq)
        self.damping = ParameterWrapper(parameter: damp)
        self.weight = ParameterWrapper(parameter: wt)
    }
}

/// Excitation parameter group
public class ExcitationParameters: ObservableObject {
    @Published public var pokeStrength: ParameterWrapper
    @Published public var pokeDuration: ParameterWrapper

    init(auParameterTree: AUParameterTree) {
        let strength = auParameterTree.parameter(withID: "excitation.pokeStrength")!
        let duration = auParameterTree.parameter(withID: "excitation.pokeDuration")!

        self.pokeStrength = ParameterWrapper(parameter: strength)
        self.pokeDuration = ParameterWrapper(parameter: duration)
    }
}

/// Voice parameter group
public class VoiceParameters: ObservableObject {
    @Published public var polyphony: ParameterWrapper
    @Published public var personality: ParameterWrapper

    init(auParameterTree: AUParameterTree) {
        let poly = auParameterTree.parameter(withID: "voice.polyphony")!
        let pers = auParameterTree.parameter(withID: "voice.personality")!

        self.polyphony = ParameterWrapper(parameter: poly)
        self.personality = ParameterWrapper(parameter: pers)
    }
}

/// Wrapper around an AUParameter that provides SwiftUI-compatible binding
public class ParameterWrapper: ObservableObject {
    private let parameter: AUParameter

    /// The current parameter value
    @Published public var value: Float {
        didSet {
            // Update the AU parameter when SwiftUI changes the value
            parameter.value = value
        }
    }

    /// The parameter's display name
    public var name: String {
        parameter.displayName
    }

    /// The parameter's identifier
    public var identifier: String {
        parameter.identifier
    }

    /// The parameter's minimum value
    public var minValue: Float {
        parameter.minValue
    }

    /// The parameter's maximum value
    public var maxValue: Float {
        parameter.maxValue
    }

    /// The parameter's unit
    public var unit: AudioUnitParameterUnit {
        parameter.unit
    }

    /// Array of value strings for indexed parameters
    public var valueStrings: [String]? {
        parameter.valueStrings
    }

    init(parameter: AUParameter) {
        self.parameter = parameter
        self.value = parameter.value

        // Observe parameter changes from DSP/host automation
        // This ensures the UI updates when the parameter changes externally
        parameter.implementorValueObserver = { [weak self] newValue in
            DispatchQueue.main.async {
                guard let self = self else { return }
                // Only update if the value actually changed to avoid feedback loops
                if self.value != newValue {
                    self.value = newValue
                }
            }
        }
    }

    /// Get a formatted string representation of the current value
    public func formattedValue() -> String {
        let value = parameter.value

        switch parameter.unit {
        case .linearGain:
            return String(format: "%.2f", value)
        case .milliseconds:
            return String(format: "%.1f ms", value)
        case .indexed:
            if let valueStrings = parameter.valueStrings,
               Int(value) < valueStrings.count {
                return valueStrings[Int(value)]
            }
            return String(format: "%.0f", value)
        default:
            return String(format: "%.2f", value)
        }
    }
}
