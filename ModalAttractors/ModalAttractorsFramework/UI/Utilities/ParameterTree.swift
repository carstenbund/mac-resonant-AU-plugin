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
    public let nodeCharacters: NodeCharacterParameters
    public let routing: RoutingParameters
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
        self.nodeCharacters = NodeCharacterParameters(auParameterTree: auParameterTree)
        self.routing = RoutingParameters(auParameterTree: auParameterTree)
        self.mode0 = ModeParameters(auParameterTree: auParameterTree, modeIndex: 0)
        self.mode1 = ModeParameters(auParameterTree: auParameterTree, modeIndex: 1)
        self.mode2 = ModeParameters(auParameterTree: auParameterTree, modeIndex: 2)
        self.mode3 = ModeParameters(auParameterTree: auParameterTree, modeIndex: 3)
        self.excitation = ExcitationParameters(auParameterTree: auParameterTree)
        self.voice = VoiceParameters(auParameterTree: auParameterTree)
    }

    /// Get wave shape parameter for a specific node and mode
    /// - Parameters:
    ///   - nodeIndex: Node index (0-4)
    ///   - modeIndex: Mode index (0-3)
    /// - Returns: ParameterWrapper for the wave shape parameter
    public func waveShapeParameter(nodeIndex: Int, modeIndex: Int) -> ParameterWrapper {
        // Calculate parameter address: base address (27) + (nodeIndex * 4) + modeIndex
        let baseAddress = ModalAttractorsExtensionParameterAddress.param_Node0_Mode0_WaveShape.rawValue
        let address = baseAddress + UInt64(nodeIndex * 4 + modeIndex)

        guard let param = auParameterTree.parameter(withAddress: address) else {
            fatalError("Invalid wave shape parameter for node \(nodeIndex), mode \(modeIndex)")
        }

        return ParameterWrapper(parameter: param)
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
        // Fetch parameters from tree by address
        let gain = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_MasterGain.rawValue)!
        let coupling = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_CouplingStrength.rawValue)!
        let topo = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_Topology.rawValue)!
        let nodes = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_NodeCount.rawValue)!

        self.masterGain = ParameterWrapper(parameter: gain)
        self.couplingStrength = ParameterWrapper(parameter: coupling)
        self.topology = ParameterWrapper(parameter: topo)
        self.nodeCount = ParameterWrapper(parameter: nodes)
    }
}

/// Node Character parameter group (5 nodes)
public class NodeCharacterParameters: ObservableObject {
    @Published public var node0: ParameterWrapper
    @Published public var node1: ParameterWrapper
    @Published public var node2: ParameterWrapper
    @Published public var node3: ParameterWrapper
    @Published public var node4: ParameterWrapper

    init(auParameterTree: AUParameterTree) {
        let n0 = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_Node0_Character.rawValue)!
        let n1 = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_Node1_Character.rawValue)!
        let n2 = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_Node2_Character.rawValue)!
        let n3 = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_Node3_Character.rawValue)!
        let n4 = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_Node4_Character.rawValue)!

        self.node0 = ParameterWrapper(parameter: n0)
        self.node1 = ParameterWrapper(parameter: n1)
        self.node2 = ParameterWrapper(parameter: n2)
        self.node3 = ParameterWrapper(parameter: n3)
        self.node4 = ParameterWrapper(parameter: n4)
    }
}

/// Routing & behavior parameter group
public class RoutingParameters: ObservableObject {
    @Published public var noteRouting: ParameterWrapper
    @Published public var multiExcite: ParameterWrapper

    init(auParameterTree: AUParameterTree) {
        let routing = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_NoteRouting.rawValue)!
        let excite = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_MultiExcite.rawValue)!

        self.noteRouting = ParameterWrapper(parameter: routing)
        self.multiExcite = ParameterWrapper(parameter: excite)
    }
}

/// Mode parameter group (generic for mode0-3)
public class ModeParameters: ObservableObject {
    @Published public var frequency: ParameterWrapper
    @Published public var damping: ParameterWrapper
    @Published public var weight: ParameterWrapper

    public let modeNumber: Int

    init(auParameterTree: AUParameterTree, modeIndex: Int) {
        self.modeNumber = modeIndex

        // Get parameter addresses based on mode index
        // Using enum values for type safety
        let freqAddr: ModalAttractorsExtensionParameterAddress
        let dampAddr: ModalAttractorsExtensionParameterAddress
        let wtAddr: ModalAttractorsExtensionParameterAddress

        switch modeIndex {
        case 0:
            freqAddr = .param_Mode0_Frequency
            dampAddr = .param_Mode0_Damping
            wtAddr = .param_Mode0_Weight
        case 1:
            freqAddr = .param_Mode1_Frequency
            dampAddr = .param_Mode1_Damping
            wtAddr = .param_Mode1_Weight
        case 2:
            freqAddr = .param_Mode2_Frequency
            dampAddr = .param_Mode2_Damping
            wtAddr = .param_Mode2_Weight
        case 3:
            freqAddr = .param_Mode3_Frequency
            dampAddr = .param_Mode3_Damping
            wtAddr = .param_Mode3_Weight
        default:
            fatalError("Invalid mode index: \(modeIndex)")
        }

        let freq = auParameterTree.parameter(withAddress: freqAddr.rawValue)!
        let damp = auParameterTree.parameter(withAddress: dampAddr.rawValue)!
        let wt = auParameterTree.parameter(withAddress: wtAddr.rawValue)!

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
        // Excitation parameters
        let strength = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_PokeStrength.rawValue)!
        let duration = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_PokeDuration.rawValue)!

        self.pokeStrength = ParameterWrapper(parameter: strength)
        self.pokeDuration = ParameterWrapper(parameter: duration)
    }
}

/// Voice parameter group
public class VoiceParameters: ObservableObject {
    @Published public var polyphony: ParameterWrapper
    @Published public var personality: ParameterWrapper

    init(auParameterTree: AUParameterTree) {
        // Voice parameters
        let poly = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_Polyphony.rawValue)!
        let pers = auParameterTree.parameter(withAddress: ModalAttractorsExtensionParameterAddress.param_Personality.rawValue)!

        self.polyphony = ParameterWrapper(parameter: poly)
        self.personality = ParameterWrapper(parameter: pers)
    }
}

/// Wrapper around an AUParameter that provides SwiftUI-compatible binding
public class ParameterWrapper: ObservableObject {
    private let parameter: AUParameter
    private var observerToken: AUParameterObserverToken?

    /// The current parameter value
    @Published public var value: Float {
        didSet {
            // Only update the parameter if it's actually different
            // This prevents feedback loops when the observer updates self.value
            if parameter.value != value {
                parameter.value = value
            }
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

        // Use token-based observation instead of implementorValueObserver
        // This allows multiple observers and doesn't override the AudioUnit's tree-level observer
        // CRITICAL: token(byAddingParameterObserver:) allows the AudioUnit's implementorValueObserver
        // to continue working while also updating the UI
        observerToken = parameter.token(byAddingParameterObserver: { [weak self] address, value in
            DispatchQueue.main.async {
                guard let self = self else { return }
                // Only update if the value actually changed
                if self.value != value {
                    self.value = value
                }
            }
        })
    }

    deinit {
        // Clean up observer token
        if let token = observerToken {
            parameter.removeParameterObserver(token)
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
