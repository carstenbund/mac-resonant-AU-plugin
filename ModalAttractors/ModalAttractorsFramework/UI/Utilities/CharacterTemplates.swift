//
//  CharacterTemplates.swift
//  ModalAttractorsFramework
//
//  Built-in character templates matching NodeCharacter.cpp
//  21 unified templates (15 original + 6 physically-derived)
//

import Foundation

/// A character template containing all parameters for a character preset
public struct CharacterTemplate {
    public let name: String
    public let description: String

    // Mode parameters: [(frequency, damping, weight)] for each of 4 modes
    public let mode0: (frequency: Float, damping: Float, weight: Float)
    public let mode1: (frequency: Float, damping: Float, weight: Float)
    public let mode2: (frequency: Float, damping: Float, weight: Float)
    public let mode3: (frequency: Float, damping: Float, weight: Float)

    // Wave shapes for each mode (0=Sine, 1=Sawtooth, 2=Triangle, 3=Square, 4=Pulse25%, 5=Pulse10%)
    public let waveShapes: (mode0: Int, mode1: Int, mode2: Int, mode3: Int)

    // Excitation parameters
    public let pokeStrength: Float
    public let pokeDuration: Float

    // Personality (0=Resonator, 1=Self-Oscillator)
    public let personality: Int

    public init(
        name: String,
        description: String = "",
        mode0: (Float, Float, Float),
        mode1: (Float, Float, Float),
        mode2: (Float, Float, Float),
        mode3: (Float, Float, Float),
        waveShapes: (Int, Int, Int, Int) = (0, 0, 0, 0),
        pokeStrength: Float,
        pokeDuration: Float,
        personality: Int = 0
    ) {
        self.name = name
        self.description = description
        self.mode0 = mode0
        self.mode1 = mode1
        self.mode2 = mode2
        self.mode3 = mode3
        self.waveShapes = waveShapes
        self.pokeStrength = pokeStrength
        self.pokeDuration = pokeDuration
        self.personality = personality
    }

    /// Load this template into a parameter store
    /// - Parameters:
    ///   - store: The parameter store to update
    ///   - nodeIndex: Node index to apply wave shapes to (0-4)
    public func apply(to store: ParameterStore, nodeIndex: Int) {
        // Mode parameters
        store.setModeParameters(0, frequency: mode0.frequency, damping: mode0.damping, weight: mode0.weight)
        store.setModeParameters(1, frequency: mode1.frequency, damping: mode1.damping, weight: mode1.weight)
        store.setModeParameters(2, frequency: mode2.frequency, damping: mode2.damping, weight: mode2.weight)
        store.setModeParameters(3, frequency: mode3.frequency, damping: mode3.damping, weight: mode3.weight)

        // Wave shapes
        store.setWaveShape(nodeIndex: nodeIndex, modeIndex: 0, waveShape: waveShapes.mode0)
        store.setWaveShape(nodeIndex: nodeIndex, modeIndex: 1, waveShape: waveShapes.mode1)
        store.setWaveShape(nodeIndex: nodeIndex, modeIndex: 2, waveShape: waveShapes.mode2)
        store.setWaveShape(nodeIndex: nodeIndex, modeIndex: 3, waveShape: waveShapes.mode3)

        // Excitation
        store.setExcitationParameters(strength: pokeStrength, duration: pokeDuration)

        // Personality
        store.setPersonality(personality)
    }
}

/// Built-in character templates matching CHARACTER_LIBRARY[] in NodeCharacter.cpp
/// Updated: 21 unified templates (was 23, removed 2 duplicates)
public struct CharacterTemplates {

    /// All available built-in templates (matches C++ CHARACTER_LIBRARY exactly)
    public static let all: [CharacterTemplate] = [
        // IDs 0-14: Original character designs
        vibrantBass,        // 0
        darkNode,           // 1
        brightBell,         // 2
        glassyShimmer,      // 3
        droneHub,           // 4
        metallicStrike,     // 5
        warmPad,            // 6
        percussiveHit,      // 7
        resonantBell,       // 8
        deepRumble,         // 9
        harmonicStack,      // 10
        detunedChorus,      // 11
        malletTone,         // 12
        windChime,          // 13
        gongWash,           // 14

        // IDs 15-20: Physically-derived presets
        churchBell,         // 15
        circularPlate,      // 16
        wineGlass,          // 17
        tunedBar,           // 18
        drumMembrane,       // 19
        smallBell           // 20
    ]

    /// Get template names
    public static var names: [String] {
        return all.map { $0.name }
    }

    /// Get template by index
    public static func template(at index: Int) -> CharacterTemplate? {
        guard index >= 0 && index < all.count else { return nil }
        return all[index]
    }

    // MARK: - Original Character Designs (0-14)

    public static let vibrantBass = CharacterTemplate(
        name: "Vibrant Bass",
        description: "Strong harmonic bass with sustained low end",
        mode0: (1.0, 0.3, 1.0),
        mode1: (2.0, 0.5, 0.8),
        mode2: (3.0, 0.8, 0.6),
        mode3: (5.0, 1.2, 0.4),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.7,
        pokeDuration: 15.0
    )

    public static let darkNode = CharacterTemplate(
        name: "Dark Node",
        description: "Muted, absorptive character with low brightness",
        mode0: (1.0, 0.8, 0.8),
        mode1: (1.5, 1.2, 0.4),
        mode2: (2.2, 1.8, 0.2),
        mode3: (3.1, 2.5, 0.1),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.4,
        pokeDuration: 8.0
    )

    public static let brightBell = CharacterTemplate(
        name: "Bright Bell",
        description: "Inharmonic bell-like tones with ringing highs",
        mode0: (1.0, 0.4, 0.7),
        mode1: (2.76, 0.6, 0.9),
        mode2: (5.40, 0.5, 1.0),
        mode3: (8.93, 0.7, 0.8),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.6,
        pokeDuration: 5.0
    )

    public static let glassyShimmer = CharacterTemplate(
        name: "Glassy Shimmer",
        description: "Airy, shimmering high partials with instability",
        mode0: (1.0, 0.5, 0.6),
        mode1: (2.01, 0.6, 0.7),
        mode2: (4.03, 0.7, 0.6),
        mode3: (11.2, 0.4, 0.9),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.5,
        pokeDuration: 12.0
    )

    public static let droneHub = CharacterTemplate(
        name: "Drone Hub",
        description: "Self-sustaining drone with beating chorus effect",
        mode0: (1.0, 0.1, 1.0),
        mode1: (1.002, 0.15, 0.9),
        mode2: (1.498, 0.2, 0.7),
        mode3: (2.0, 0.3, 0.5),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.3,
        pokeDuration: 20.0,
        personality: 1  // Self-Oscillator
    )

    public static let metallicStrike = CharacterTemplate(
        name: "Metallic Strike",
        description: "Bright inharmonic strike with fast decay",
        mode0: (1.0, 2.0, 0.6),
        mode1: (3.14, 2.5, 0.8),
        mode2: (5.87, 3.0, 1.0),
        mode3: (8.23, 3.5, 0.7),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.9,
        pokeDuration: 5.0
    )

    public static let warmPad = CharacterTemplate(
        name: "Warm Pad",
        description: "Smooth sustained pad with perfect harmonics",
        mode0: (1.0, 0.2, 1.0),
        mode1: (2.0, 0.25, 0.85),
        mode2: (3.0, 0.3, 0.7),
        mode3: (4.0, 0.4, 0.5),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.3,
        pokeDuration: 30.0
    )

    public static let percussiveHit = CharacterTemplate(
        name: "Percussive Hit",
        description: "Fast decay percussive strike",
        mode0: (1.0, 3.0, 1.0),
        mode1: (2.5, 3.5, 0.6),
        mode2: (4.2, 4.0, 0.4),
        mode3: (6.7, 4.5, 0.2),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 1.0,
        pokeDuration: 3.0
    )

    public static let resonantBell = CharacterTemplate(
        name: "Resonant Bell",
        description: "Harmonic bell with balanced sustain",
        mode0: (1.0, 0.6, 1.0),
        mode1: (2.0, 0.7, 0.9),
        mode2: (3.0, 0.8, 0.8),
        mode3: (4.0, 1.0, 0.7),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.75,
        pokeDuration: 12.0
    )

    public static let deepRumble = CharacterTemplate(
        name: "Deep Rumble",
        description: "Sub-bass focus with low partials",
        mode0: (0.5, 0.5, 1.0),
        mode1: (1.0, 0.6, 0.9),
        mode2: (1.5, 0.8, 0.6),
        mode3: (2.0, 1.0, 0.4),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.6,
        pokeDuration: 20.0
    )

    public static let harmonicStack = CharacterTemplate(
        name: "Harmonic Stack",
        description: "Perfect harmonic series with uniform damping",
        mode0: (1.0, 1.0, 1.0),
        mode1: (2.0, 1.0, 0.8),
        mode2: (3.0, 1.0, 0.6),
        mode3: (4.0, 1.0, 0.4),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.65,
        pokeDuration: 15.0
    )

    public static let detunedChorus = CharacterTemplate(
        name: "Detuned Chorus",
        description: "Slightly detuned for thick chorused sound",
        mode0: (1.0, 0.7, 1.0),
        mode1: (1.99, 0.7, 0.85),
        mode2: (2.98, 0.8, 0.7),
        mode3: (4.03, 0.9, 0.5),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.5,
        pokeDuration: 18.0
    )

    public static let malletTone = CharacterTemplate(
        name: "Mallet Tone",
        description: "Wood mallet-like inharmonic character",
        mode0: (1.0, 1.5, 1.0),
        mode1: (2.76, 1.8, 0.7),
        mode2: (4.18, 2.2, 0.5),
        mode3: (5.94, 2.5, 0.3),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.85,
        pokeDuration: 8.0
    )

    public static let windChime = CharacterTemplate(
        name: "Wind Chime",
        description: "High delicate partials, light and airy",
        mode0: (3.0, 0.9, 0.7),
        mode1: (4.5, 1.0, 0.8),
        mode2: (6.2, 1.1, 1.0),
        mode3: (8.7, 1.3, 0.8),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.4,
        pokeDuration: 14.0
    )

    public static let gongWash = CharacterTemplate(
        name: "Gong Wash",
        description: "Complex inharmonic wash with long sustain",
        mode0: (1.0, 0.4, 0.8),
        mode1: (2.37, 0.5, 1.0),
        mode2: (3.86, 0.6, 0.9),
        mode3: (5.19, 0.7, 0.7),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.7,
        pokeDuration: 35.0
    )

    // MARK: - Physically-Derived Presets (15-20)
    // Based on acoustics literature (Fletcher & Rossing, Rayleigh)

    public static let churchBell = CharacterTemplate(
        name: "Church Bell",
        description: "Western church bell with hum, fundamental, tierce, and quint",
        mode0: (1.0, 1.0, 1.0),
        mode1: (1.19, 1.2, 0.75),
        mode2: (1.5, 1.4, 0.6),
        mode3: (2.0, 1.8, 0.45),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.7,
        pokeDuration: 10.0
    )

    public static let circularPlate = CharacterTemplate(
        name: "Circular Plate",
        description: "Flat circular plate (cymbal, gong) - Rayleigh modes",
        mode0: (1.0, 1.0, 1.0),
        mode1: (2.081, 1.1, 0.7),
        mode2: (3.413, 1.3, 0.5),
        mode3: (3.891, 1.4, 0.35),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.8,
        pokeDuration: 8.0
    )

    public static let wineGlass = CharacterTemplate(
        name: "Wine Glass",
        description: "Cylindrical shell resonance (wine glass rim)",
        mode0: (1.0, 1.0, 1.0),
        mode1: (2.28, 1.3, 0.65),
        mode2: (3.65, 1.6, 0.45),
        mode3: (5.13, 2.0, 0.3),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.6,
        pokeDuration: 12.0
    )

    public static let tunedBar = CharacterTemplate(
        name: "Tuned Bar",
        description: "Arch-tuned bar (marimba, xylophone) with harmonic overtones",
        mode0: (1.0, 1.0, 1.0),
        mode1: (4.0, 1.4, 0.5),
        mode2: (10.0, 2.0, 0.25),
        mode3: (18.0, 2.5, 0.15),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.75,
        pokeDuration: 10.0
    )

    public static let drumMembrane = CharacterTemplate(
        name: "Drum Membrane",
        description: "Circular membrane (kettledrum, timpani)",
        mode0: (1.0, 0.8, 1.0),
        mode1: (1.593, 1.0, 0.7),
        mode2: (2.136, 1.2, 0.5),
        mode3: (2.296, 1.3, 0.4),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.8,
        pokeDuration: 15.0
    )

    public static let smallBell = CharacterTemplate(
        name: "Small Bell",
        description: "Small handbell or bicycle bell (higher inharmonicity)",
        mode0: (1.0, 0.6, 1.0),
        mode1: (1.35, 0.8, 0.8),
        mode2: (1.7, 1.0, 0.6),
        mode3: (2.2, 1.2, 0.4),
        waveShapes: (0, 0, 0, 0),
        pokeStrength: 0.7,
        pokeDuration: 7.0
    )
}
