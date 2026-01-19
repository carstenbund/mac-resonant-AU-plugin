//
//  FactoryPresets.swift
//  ModalAttractorsExtension
//
//  Factory preset definitions for AUv3 preset system integration
//  Contains all parameter values for factory presets that can be browsed
//  from DAW hosts like Logic Pro and GarageBand
//

import AudioToolbox

/// Factory preset definition containing all character parameters
public struct FactoryPresetData {
    let name: String

    // Mode parameters (4 modes × 3 values each)
    let mode0: (frequency: Float, damping: Float, weight: Float)
    let mode1: (frequency: Float, damping: Float, weight: Float)
    let mode2: (frequency: Float, damping: Float, weight: Float)
    let mode3: (frequency: Float, damping: Float, weight: Float)

    // Wave shapes for each mode (0=Sine, 1=Sawtooth, 2=Triangle, 3=Square, 4=Pulse25%, 5=Pulse10%)
    let mode0WaveShape: Int
    let mode1WaveShape: Int
    let mode2WaveShape: Int
    let mode3WaveShape: Int

    // Excitation parameters
    let pokeStrength: Float
    let pokeDuration: Float

    // Voice personality (0-1)
    let personality: Int

    /// Convert to state dictionary compatible with fullState
    func toStateDictionary() -> [String: Any] {
        return [
            // Mode parameters
            "mode0Frequency": mode0.frequency,
            "mode0Damping": mode0.damping,
            "mode0Weight": mode0.weight,
            "mode1Frequency": mode1.frequency,
            "mode1Damping": mode1.damping,
            "mode1Weight": mode1.weight,
            "mode2Frequency": mode2.frequency,
            "mode2Damping": mode2.damping,
            "mode2Weight": mode2.weight,
            "mode3Frequency": mode3.frequency,
            "mode3Damping": mode3.damping,
            "mode3Weight": mode3.weight,

            // Excitation
            "pokeStrength": pokeStrength,
            "pokeDuration": pokeDuration,

            // Personality
            "personality": Float(personality)
        ]
    }

    /// Convert to state dictionary including wave shapes for a specific node
    /// - Parameter nodeIndex: The node index (0-4) to apply wave shapes to
    func toStateDictionary(forNode nodeIndex: Int) -> [String: Any] {
        var state = toStateDictionary()

        // Add wave shapes for the specified node
        state["node\(nodeIndex)Mode0WaveShape"] = Float(mode0WaveShape)
        state["node\(nodeIndex)Mode1WaveShape"] = Float(mode1WaveShape)
        state["node\(nodeIndex)Mode2WaveShape"] = Float(mode2WaveShape)
        state["node\(nodeIndex)Mode3WaveShape"] = Float(mode3WaveShape)

        return state
    }
}

/// All 15 factory presets matching CharacterTemplates.swift
let ModalAttractorsFactoryPresets: [FactoryPresetData] = [
    // 0: Vibrant Bass
    FactoryPresetData(
        name: "Vibrant Bass",
        mode0: (1.0, 0.8, 1.0),
        mode1: (2.01, 1.0, 0.7),
        mode2: (3.02, 1.2, 0.5),
        mode3: (4.05, 1.5, 0.3),
        mode0WaveShape: 0, // Sine
        mode1WaveShape: 1, // Sawtooth
        mode2WaveShape: 2, // Triangle
        mode3WaveShape: 0, // Sine
        pokeStrength: 0.7,
        pokeDuration: 15.0,
        personality: 0
    ),

    // 1: Dark Node
    FactoryPresetData(
        name: "Dark Node",
        mode0: (1.0, 0.5, 1.0),
        mode1: (1.9, 0.6, 0.8),
        mode2: (2.8, 0.7, 0.6),
        mode3: (3.5, 0.9, 0.4),
        mode0WaveShape: 1, // Sawtooth
        mode1WaveShape: 3, // Square
        mode2WaveShape: 1, // Sawtooth
        mode3WaveShape: 2, // Triangle
        pokeStrength: 0.5,
        pokeDuration: 20.0,
        personality: 0
    ),

    // 2: Bright Bell
    FactoryPresetData(
        name: "Bright Bell",
        mode0: (1.0, 1.2, 0.8),
        mode1: (2.0, 1.4, 1.0),
        mode2: (3.0, 1.6, 0.7),
        mode3: (4.0, 2.0, 0.5),
        mode0WaveShape: 0, // Sine
        mode1WaveShape: 0, // Sine
        mode2WaveShape: 0, // Sine
        mode3WaveShape: 0, // Sine
        pokeStrength: 0.8,
        pokeDuration: 10.0,
        personality: 0
    ),

    // 3: Glassy Shimmer
    FactoryPresetData(
        name: "Glassy Shimmer",
        mode0: (2.0, 0.8, 0.6),
        mode1: (3.5, 1.0, 0.8),
        mode2: (5.2, 1.2, 1.0),
        mode3: (7.1, 1.5, 0.7),
        mode0WaveShape: 2, // Triangle
        mode1WaveShape: 0, // Sine
        mode2WaveShape: 2, // Triangle
        mode3WaveShape: 2, // Triangle
        pokeStrength: 0.6,
        pokeDuration: 12.0,
        personality: 0
    ),

    // 4: Drone Hub
    FactoryPresetData(
        name: "Drone Hub",
        mode0: (1.0, 0.3, 1.0),
        mode1: (1.5, 0.4, 0.9),
        mode2: (2.2, 0.5, 0.8),
        mode3: (3.1, 0.6, 0.7),
        mode0WaveShape: 3, // Square
        mode1WaveShape: 0, // Sine
        mode2WaveShape: 3, // Square
        mode3WaveShape: 1, // Sawtooth
        pokeStrength: 0.4,
        pokeDuration: 25.0,
        personality: 0
    ),

    // 5: Metallic Strike
    FactoryPresetData(
        name: "Metallic Strike",
        mode0: (1.0, 2.0, 0.6),
        mode1: (3.14, 2.5, 0.8),
        mode2: (5.87, 3.0, 1.0),
        mode3: (8.23, 3.5, 0.7),
        mode0WaveShape: 3, // Square
        mode1WaveShape: 1, // Sawtooth
        mode2WaveShape: 3, // Square
        mode3WaveShape: 4, // Pulse25%
        pokeStrength: 0.9,
        pokeDuration: 5.0,
        personality: 0
    ),

    // 6: Warm Pad
    FactoryPresetData(
        name: "Warm Pad",
        mode0: (1.0, 0.2, 1.0),
        mode1: (2.0, 0.25, 0.85),
        mode2: (3.0, 0.3, 0.7),
        mode3: (4.0, 0.4, 0.5),
        mode0WaveShape: 0, // Sine
        mode1WaveShape: 2, // Triangle
        mode2WaveShape: 0, // Sine
        mode3WaveShape: 2, // Triangle
        pokeStrength: 0.3,
        pokeDuration: 30.0,
        personality: 0
    ),

    // 7: Percussive Hit
    FactoryPresetData(
        name: "Percussive Hit",
        mode0: (1.0, 3.0, 1.0),
        mode1: (2.5, 3.5, 0.6),
        mode2: (4.2, 4.0, 0.4),
        mode3: (6.7, 4.5, 0.2),
        mode0WaveShape: 5, // Pulse10%
        mode1WaveShape: 3, // Square
        mode2WaveShape: 4, // Pulse25%
        mode3WaveShape: 1, // Sawtooth
        pokeStrength: 1.0,
        pokeDuration: 3.0,
        personality: 0
    ),

    // 8: Resonant Bell
    FactoryPresetData(
        name: "Resonant Bell",
        mode0: (1.0, 0.6, 1.0),
        mode1: (2.0, 0.7, 0.9),
        mode2: (3.0, 0.8, 0.8),
        mode3: (4.0, 1.0, 0.7),
        mode0WaveShape: 0, // Sine
        mode1WaveShape: 0, // Sine
        mode2WaveShape: 2, // Triangle
        mode3WaveShape: 0, // Sine
        pokeStrength: 0.75,
        pokeDuration: 12.0,
        personality: 0
    ),

    // 9: Deep Rumble
    FactoryPresetData(
        name: "Deep Rumble",
        mode0: (0.5, 0.5, 1.0),
        mode1: (1.0, 0.6, 0.9),
        mode2: (1.5, 0.8, 0.6),
        mode3: (2.0, 1.0, 0.4),
        mode0WaveShape: 0, // Sine
        mode1WaveShape: 1, // Sawtooth
        mode2WaveShape: 0, // Sine
        mode3WaveShape: 1, // Sawtooth
        pokeStrength: 0.6,
        pokeDuration: 20.0,
        personality: 0
    ),

    // 10: Harmonic Stack
    FactoryPresetData(
        name: "Harmonic Stack",
        mode0: (1.0, 1.0, 1.0),
        mode1: (2.0, 1.0, 0.8),
        mode2: (3.0, 1.0, 0.6),
        mode3: (4.0, 1.0, 0.4),
        mode0WaveShape: 0, // Sine
        mode1WaveShape: 0, // Sine
        mode2WaveShape: 0, // Sine
        mode3WaveShape: 0, // Sine
        pokeStrength: 0.65,
        pokeDuration: 15.0,
        personality: 0
    ),

    // 11: Detuned Chorus
    FactoryPresetData(
        name: "Detuned Chorus",
        mode0: (1.0, 0.7, 1.0),
        mode1: (1.99, 0.7, 0.85),
        mode2: (2.98, 0.8, 0.7),
        mode3: (4.03, 0.9, 0.5),
        mode0WaveShape: 1, // Sawtooth
        mode1WaveShape: 1, // Sawtooth
        mode2WaveShape: 2, // Triangle
        mode3WaveShape: 1, // Sawtooth
        pokeStrength: 0.5,
        pokeDuration: 18.0,
        personality: 0
    ),

    // 12: Mallet Tone
    FactoryPresetData(
        name: "Mallet Tone",
        mode0: (1.0, 1.5, 1.0),
        mode1: (2.76, 1.8, 0.7),
        mode2: (4.18, 2.2, 0.5),
        mode3: (5.94, 2.5, 0.3),
        mode0WaveShape: 2, // Triangle
        mode1WaveShape: 3, // Square
        mode2WaveShape: 1, // Sawtooth
        mode3WaveShape: 2, // Triangle
        pokeStrength: 0.85,
        pokeDuration: 8.0,
        personality: 0
    ),

    // 13: Wind Chime
    FactoryPresetData(
        name: "Wind Chime",
        mode0: (3.0, 0.9, 0.7),
        mode1: (4.5, 1.0, 0.8),
        mode2: (6.2, 1.1, 1.0),
        mode3: (8.7, 1.3, 0.8),
        mode0WaveShape: 2, // Triangle
        mode1WaveShape: 2, // Triangle
        mode2WaveShape: 0, // Sine
        mode3WaveShape: 2, // Triangle
        pokeStrength: 0.4,
        pokeDuration: 14.0,
        personality: 0
    ),

    // 14: Gong Wash
    FactoryPresetData(
        name: "Gong Wash",
        mode0: (1.0, 0.4, 0.8),
        mode1: (2.37, 0.5, 1.0),
        mode2: (3.86, 0.6, 0.9),
        mode3: (5.19, 0.7, 0.7),
        mode0WaveShape: 1, // Sawtooth
        mode1WaveShape: 3, // Square
        mode2WaveShape: 1, // Sawtooth
        mode3WaveShape: 4, // Pulse25%
        pokeStrength: 0.7,
        pokeDuration: 35.0,
        personality: 0
    )
]
