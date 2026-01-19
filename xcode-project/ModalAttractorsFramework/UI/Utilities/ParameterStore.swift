//
//  ParameterStore.swift
//  ModalAttractorsFramework
//
//  Parameter storage system compatible with Logic's fullState preset system
//  Separates data storage from UI rendering concerns
//

import Foundation
import AudioToolbox
import Combine

/// Parameter store that manages parameter values in a format compatible with
/// the Audio Unit's fullState dictionary system used by Logic Pro and other DAWs.
///
/// This provides:
/// - Clean separation between data storage and UI rendering
/// - Direct compatibility with AUAudioUnit.fullState
/// - Type-safe accessors for parameter groups
/// - Efficient serialization for presets
public class ParameterStore: ObservableObject {

    /// Internal storage using parameter identifiers as keys
    /// This matches the format used by AUAudioUnit.fullState
    @Published private(set) var values: [String: Float] = [:]

    // MARK: - Initialization

    /// Create an empty parameter store
    public init() {
        self.values = [:]
    }

    /// Create a parameter store from a fullState dictionary
    /// - Parameter fullState: Dictionary from AUAudioUnit.fullState
    public init(fromFullState fullState: [String: Any]) {
        self.values = fullState.compactMapValues { $0 as? Float }
    }

    /// Create a parameter store from a parameter tree
    /// - Parameter parameterTree: The Audio Unit's parameter tree
    public convenience init(from parameterTree: AUParameterTree) {
        self.init()
        for param in parameterTree.allParameters {
            values[param.identifier] = param.value
        }
    }

    /// Create a parameter store from a ParameterTree wrapper
    /// - Parameter parameterTree: The ParameterTree wrapper
    public convenience init(from parameterTree: ParameterTree) {
        self.init()
        self.loadFrom(parameterTree)
    }

    // MARK: - Value Access

    /// Get a parameter value by identifier
    /// - Parameter identifier: The parameter identifier (e.g., "mode0Frequency")
    /// - Returns: The parameter value, or nil if not found
    public func getValue(_ identifier: String) -> Float? {
        return values[identifier]
    }

    /// Set a parameter value by identifier
    /// - Parameters:
    ///   - identifier: The parameter identifier
    ///   - value: The new value
    public func setValue(_ identifier: String, value: Float) {
        values[identifier] = value
    }

    /// Get multiple values at once
    /// - Parameter identifiers: Array of parameter identifiers
    /// - Returns: Dictionary of identifier -> value
    public func getValues(_ identifiers: [String]) -> [String: Float] {
        return identifiers.reduce(into: [:]) { result, id in
            if let value = values[id] {
                result[id] = value
            }
        }
    }

    /// Set multiple values at once
    /// - Parameter newValues: Dictionary of identifier -> value
    public func setValues(_ newValues: [String: Float]) {
        for (key, value) in newValues {
            values[key] = value
        }
    }

    // MARK: - Type-Safe Accessors (Mode Parameters)

    /// Get mode parameters for a specific mode
    /// - Parameter modeIndex: Mode index (0-3)
    /// - Returns: Tuple of (frequency, damping, weight)
    public func getModeParameters(_ modeIndex: Int) -> (frequency: Float, damping: Float, weight: Float)? {
        let freqId = "mode\(modeIndex)Frequency"
        let dampId = "mode\(modeIndex)Damping"
        let wtId = "mode\(modeIndex)Weight"

        guard let freq = values[freqId],
              let damp = values[dampId],
              let wt = values[wtId] else {
            return nil
        }

        return (freq, damp, wt)
    }

    /// Set mode parameters for a specific mode
    /// - Parameters:
    ///   - modeIndex: Mode index (0-3)
    ///   - frequency: Frequency multiplier
    ///   - damping: Damping coefficient
    ///   - weight: Mode weight/amplitude
    public func setModeParameters(_ modeIndex: Int, frequency: Float, damping: Float, weight: Float) {
        values["mode\(modeIndex)Frequency"] = frequency
        values["mode\(modeIndex)Damping"] = damping
        values["mode\(modeIndex)Weight"] = weight
    }

    /// Get wave shape for a specific node and mode
    /// - Parameters:
    ///   - nodeIndex: Node index (0-4)
    ///   - modeIndex: Mode index (0-3)
    /// - Returns: Wave shape index (0-5)
    public func getWaveShape(nodeIndex: Int, modeIndex: Int) -> Int? {
        let id = "node\(nodeIndex)Mode\(modeIndex)WaveShape"
        return values[id].map { Int($0) }
    }

    /// Set wave shape for a specific node and mode
    /// - Parameters:
    ///   - nodeIndex: Node index (0-4)
    ///   - modeIndex: Mode index (0-3)
    ///   - waveShape: Wave shape index (0-5)
    public func setWaveShape(nodeIndex: Int, modeIndex: Int, waveShape: Int) {
        let id = "node\(nodeIndex)Mode\(modeIndex)WaveShape"
        values[id] = Float(waveShape)
    }

    // MARK: - Type-Safe Accessors (Excitation)

    /// Get excitation parameters
    /// - Returns: Tuple of (strength, duration)
    public func getExcitationParameters() -> (strength: Float, duration: Float)? {
        guard let strength = values["pokeStrength"],
              let duration = values["pokeDuration"] else {
            return nil
        }
        return (strength, duration)
    }

    /// Set excitation parameters
    /// - Parameters:
    ///   - strength: Poke strength
    ///   - duration: Poke duration in ms
    public func setExcitationParameters(strength: Float, duration: Float) {
        values["pokeStrength"] = strength
        values["pokeDuration"] = duration
    }

    // MARK: - Type-Safe Accessors (Personality)

    /// Get personality parameter
    /// - Returns: Personality index
    public func getPersonality() -> Int? {
        return values["personality"].map { Int($0) }
    }

    /// Set personality parameter
    /// - Parameter personality: Personality index
    public func setPersonality(_ personality: Int) {
        values["personality"] = Float(personality)
    }

    // MARK: - Bulk Operations

    /// Load all values from a parameter tree
    /// - Parameter parameterTree: The ParameterTree to read from
    public func loadFrom(_ parameterTree: ParameterTree) {
        // Mode parameters
        for i in 0..<4 {
            let mode = [parameterTree.mode0, parameterTree.mode1,
                       parameterTree.mode2, parameterTree.mode3][i]
            values["mode\(i)Frequency"] = mode.frequency.value
            values["mode\(i)Damping"] = mode.damping.value
            values["mode\(i)Weight"] = mode.weight.value
        }

        // Excitation
        values["pokeStrength"] = parameterTree.excitation.pokeStrength.value
        values["pokeDuration"] = parameterTree.excitation.pokeDuration.value

        // Personality
        values["personality"] = parameterTree.voice.personality.value

        // Wave shapes for all nodes and modes
        for nodeIndex in 0..<5 {
            for modeIndex in 0..<4 {
                let waveShape = parameterTree.waveShapeParameter(
                    nodeIndex: nodeIndex,
                    modeIndex: modeIndex
                ).value
                values["node\(nodeIndex)Mode\(modeIndex)WaveShape"] = waveShape
            }
        }
    }

    /// Apply all values to a parameter tree
    /// - Parameters:
    ///   - parameterTree: The ParameterTree to update
    ///   - nodeIndex: Optional node index to apply wave shapes to (if nil, applies to all nodes)
    public func applyTo(_ parameterTree: ParameterTree, nodeIndex: Int? = nil) {
        // Mode parameters
        if let freq = values["mode0Frequency"] { parameterTree.mode0.frequency.value = freq }
        if let damp = values["mode0Damping"] { parameterTree.mode0.damping.value = damp }
        if let wt = values["mode0Weight"] { parameterTree.mode0.weight.value = wt }

        if let freq = values["mode1Frequency"] { parameterTree.mode1.frequency.value = freq }
        if let damp = values["mode1Damping"] { parameterTree.mode1.damping.value = damp }
        if let wt = values["mode1Weight"] { parameterTree.mode1.weight.value = wt }

        if let freq = values["mode2Frequency"] { parameterTree.mode2.frequency.value = freq }
        if let damp = values["mode2Damping"] { parameterTree.mode2.damping.value = damp }
        if let wt = values["mode2Weight"] { parameterTree.mode2.weight.value = wt }

        if let freq = values["mode3Frequency"] { parameterTree.mode3.frequency.value = freq }
        if let damp = values["mode3Damping"] { parameterTree.mode3.damping.value = damp }
        if let wt = values["mode3Weight"] { parameterTree.mode3.weight.value = wt }

        // Excitation
        if let strength = values["pokeStrength"] {
            parameterTree.excitation.pokeStrength.value = strength
        }
        if let duration = values["pokeDuration"] {
            parameterTree.excitation.pokeDuration.value = duration
        }

        // Personality
        if let personality = values["personality"] {
            parameterTree.voice.personality.value = personality
        }

        // Wave shapes - apply to specific node or all nodes
        let nodeIndices = nodeIndex.map { [$0] } ?? Array(0..<5)
        for node in nodeIndices {
            for mode in 0..<4 {
                let id = "node\(node)Mode\(mode)WaveShape"
                if let waveShape = values[id] {
                    parameterTree.waveShapeParameter(
                        nodeIndex: node,
                        modeIndex: mode
                    ).value = waveShape
                }
            }
        }
    }

    // MARK: - Serialization (Compatible with fullState)

    /// Convert to a dictionary compatible with AUAudioUnit.fullState
    /// - Returns: Dictionary of parameter identifier -> value
    public func toFullState() -> [String: Any] {
        return values
    }

    /// Load from a fullState dictionary
    /// - Parameter fullState: Dictionary from AUAudioUnit.fullState
    public func loadFromFullState(_ fullState: [String: Any]) {
        self.values = fullState.compactMapValues { $0 as? Float }
    }

    // MARK: - Codable Support

    /// Encode to JSON data
    /// - Returns: JSON data
    public func encode() throws -> Data {
        let encoder = JSONEncoder()
        encoder.outputFormatting = .prettyPrinted
        return try encoder.encode(values)
    }

    /// Decode from JSON data
    /// - Parameter data: JSON data
    public func decode(from data: Data) throws {
        let decoder = JSONDecoder()
        self.values = try decoder.decode([String: Float].self, from: data)
    }
}
