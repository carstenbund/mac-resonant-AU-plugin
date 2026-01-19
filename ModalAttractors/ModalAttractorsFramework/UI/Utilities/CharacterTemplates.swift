//
//  CharacterTemplates.swift
//  ModalAttractorsFramework
//
//  Built-in character templates for the character editor
//  Separated from UI code for better organization
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

    // Personality (optional, defaults to 0 if not specified)
    public let personality: Int

    public init(
        name: String,
        description: String = "",
        mode0: (Float, Float, Float),
        mode1: (Float, Float, Float),
        mode2: (Float, Float, Float),
        mode3: (Float, Float, Float),
        waveShapes: (Int, Int, Int, Int),
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

/// Built-in character templates matching NodeCharacter.cpp definitions
public struct CharacterTemplates {

    /// All available built-in templates
    public static let all: [CharacterTemplate] = [
        // Original presets
        vibrantBass,
        darkNode,
        brightBell,
        glassyShimmer,
        droneHub,
        metallicStrike,
        warmPad,
        percussiveHit,
        resonantBell,
        deepRumble,
        harmonicStack,
        detunedChorus,
        malletTone,
        windChime,
        gongWash,

        // Physically-derived presets (based on acoustics literature)
        churchBell,
        circularPlate,
        wineGlass,
        freeBar,
        tunedBar,
        drumMembrane,
        smallBell,
        harmonicString
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

    // MARK: - Template Definitions

    public static let vibrantBass = CharacterTemplate(
        name: "Vibrant Bass",
        description: "Low, rich fundamentals with harmonic content",
        mode0: (1.0, 0.8, 1.0),
        mode1: (2.01, 1.0, 0.7),
        mode2: (3.02, 1.2, 0.5),
        mode3: (4.05, 1.5, 0.3),
        waveShapes: (0, 1, 2, 0), // Sine, Sawtooth, Triangle, Sine
        pokeStrength: 0.7,
        pokeDuration: 15.0
    )

    public static let darkNode = CharacterTemplate(
        name: "Dark Node",
        description: "Low damping, complex with sawtooth character",
        mode0: (1.0, 0.5, 1.0),
        mode1: (1.9, 0.6, 0.8),
        mode2: (2.8, 0.7, 0.6),
        mode3: (3.5, 0.9, 0.4),
        waveShapes: (1, 3, 1, 2), // Sawtooth, Square, Sawtooth, Triangle
        pokeStrength: 0.5,
        pokeDuration: 20.0
    )

    public static let brightBell = CharacterTemplate(
        name: "Bright Bell",
        description: "Harmonic, ringing with pure sine waves",
        mode0: (1.0, 1.2, 0.8),
        mode1: (2.0, 1.4, 1.0),
        mode2: (3.0, 1.6, 0.7),
        mode3: (4.0, 2.0, 0.5),
        waveShapes: (0, 0, 0, 0), // All Sine
        pokeStrength: 0.8,
        pokeDuration: 10.0
    )

    public static let glassyShimmer = CharacterTemplate(
        name: "Glassy Shimmer",
        description: "High partials with triangle smoothness",
        mode0: (2.0, 0.8, 0.6),
        mode1: (3.5, 1.0, 0.8),
        mode2: (5.2, 1.2, 1.0),
        mode3: (7.1, 1.5, 0.7),
        waveShapes: (2, 0, 2, 2), // Triangle, Sine, Triangle, Triangle
        pokeStrength: 0.6,
        pokeDuration: 12.0
    )

    public static let droneHub = CharacterTemplate(
        name: "Drone Hub",
        description: "Sustained, coupled with square character",
        mode0: (1.0, 0.3, 1.0),
        mode1: (1.5, 0.4, 0.9),
        mode2: (2.2, 0.5, 0.8),
        mode3: (3.1, 0.6, 0.7),
        waveShapes: (3, 0, 3, 1), // Square, Sine, Square, Sawtooth
        pokeStrength: 0.4,
        pokeDuration: 25.0
    )

    public static let metallicStrike = CharacterTemplate(
        name: "Metallic Strike",
        description: "Bright, sharp attack with fast decay",
        mode0: (1.0, 2.0, 0.6),
        mode1: (3.14, 2.5, 0.8),
        mode2: (5.87, 3.0, 1.0),
        mode3: (8.23, 3.5, 0.7),
        waveShapes: (3, 1, 3, 4), // Square, Sawtooth, Square, Pulse25%
        pokeStrength: 0.9,
        pokeDuration: 5.0
    )

    public static let warmPad = CharacterTemplate(
        name: "Warm Pad",
        description: "Smooth, sustained with low harmonics",
        mode0: (1.0, 0.2, 1.0),
        mode1: (2.0, 0.25, 0.85),
        mode2: (3.0, 0.3, 0.7),
        mode3: (4.0, 0.4, 0.5),
        waveShapes: (0, 2, 0, 2), // Sine, Triangle, Sine, Triangle
        pokeStrength: 0.3,
        pokeDuration: 30.0
    )

    public static let percussiveHit = CharacterTemplate(
        name: "Percussive Hit",
        description: "Fast decay, punchy with pulse character",
        mode0: (1.0, 3.0, 1.0),
        mode1: (2.5, 3.5, 0.6),
        mode2: (4.2, 4.0, 0.4),
        mode3: (6.7, 4.5, 0.2),
        waveShapes: (5, 3, 4, 1), // Pulse10%, Square, Pulse25%, Sawtooth
        pokeStrength: 1.0,
        pokeDuration: 3.0
    )

    public static let resonantBell = CharacterTemplate(
        name: "Resonant Bell",
        description: "Harmonic stack with long sustain",
        mode0: (1.0, 0.6, 1.0),
        mode1: (2.0, 0.7, 0.9),
        mode2: (3.0, 0.8, 0.8),
        mode3: (4.0, 1.0, 0.7),
        waveShapes: (0, 0, 2, 0), // Sine, Sine, Triangle, Sine
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
        waveShapes: (0, 1, 0, 1), // Sine, Sawtooth, Sine, Sawtooth
        pokeStrength: 0.6,
        pokeDuration: 20.0
    )

    public static let harmonicStack = CharacterTemplate(
        name: "Harmonic Stack",
        description: "Perfect harmonic series with sine waves",
        mode0: (1.0, 1.0, 1.0),
        mode1: (2.0, 1.0, 0.8),
        mode2: (3.0, 1.0, 0.6),
        mode3: (4.0, 1.0, 0.4),
        waveShapes: (0, 0, 0, 0), // All Sine
        pokeStrength: 0.65,
        pokeDuration: 15.0
    )

    public static let detunedChorus = CharacterTemplate(
        name: "Detuned Chorus",
        description: "Slightly detuned with thick sawtooth",
        mode0: (1.0, 0.7, 1.0),
        mode1: (1.99, 0.7, 0.85),
        mode2: (2.98, 0.8, 0.7),
        mode3: (4.03, 0.9, 0.5),
        waveShapes: (1, 1, 2, 1), // Sawtooth, Sawtooth, Triangle, Sawtooth
        pokeStrength: 0.5,
        pokeDuration: 18.0
    )

    public static let malletTone = CharacterTemplate(
        name: "Mallet Tone",
        description: "Wood/mallet character with mixed waves",
        mode0: (1.0, 1.5, 1.0),
        mode1: (2.76, 1.8, 0.7),
        mode2: (4.18, 2.2, 0.5),
        mode3: (5.94, 2.5, 0.3),
        waveShapes: (2, 3, 1, 2), // Triangle, Square, Sawtooth, Triangle
        pokeStrength: 0.85,
        pokeDuration: 8.0
    )

    public static let windChime = CharacterTemplate(
        name: "Wind Chime",
        description: "Delicate, high partials with triangle",
        mode0: (3.0, 0.9, 0.7),
        mode1: (4.5, 1.0, 0.8),
        mode2: (6.2, 1.1, 1.0),
        mode3: (8.7, 1.3, 0.8),
        waveShapes: (2, 2, 0, 2), // Triangle, Triangle, Sine, Triangle
        pokeStrength: 0.4,
        pokeDuration: 14.0
    )

    public static let gongWash = CharacterTemplate(
        name: "Gong Wash",
        description: "Complex inharmonic with evolving character",
        mode0: (1.0, 0.4, 0.8),
        mode1: (2.37, 0.5, 1.0),
        mode2: (3.86, 0.6, 0.9),
        mode3: (5.19, 0.7, 0.7),
        waveShapes: (1, 3, 1, 4), // Sawtooth, Square, Sawtooth, Pulse25%
        pokeStrength: 0.7,
        pokeDuration: 35.0
    )

    // MARK: - Physically-Derived Presets
    // Based on acoustics literature (Fletcher & Rossing, Rayleigh)

    public static let churchBell = CharacterTemplate(
        name: "Church Bell",
        description: "Western church bell with tierce (minor third) character",
        mode0: (1.000, 0.5, 1.00),   // Fundamental
        mode1: (1.190, 0.6, 0.75),   // Tierce (minor third - classic bell sound)
        mode2: (1.500, 0.7, 0.60),   // Quint (perfect fifth)
        mode3: (2.000, 0.9, 0.45),   // Nominal (octave)
        waveShapes: (0, 0, 0, 0),    // All sine for pure bell tone
        pokeStrength: 0.8,
        pokeDuration: 10.0
    )

    public static let circularPlate = CharacterTemplate(
        name: "Circular Plate",
        description: "Cymbal/gong with Rayleigh modes (inharmonic)",
        mode0: (1.000, 0.5, 1.00),   // (0,1) mode
        mode1: (2.081, 0.55, 0.70),  // (1,1) mode - Rayleigh
        mode2: (3.413, 0.65, 0.50),  // (2,1) mode - Rayleigh
        mode3: (3.891, 0.70, 0.35),  // (0,2) mode - Rayleigh
        waveShapes: (0, 0, 0, 0),    // Pure sine for metallic clarity
        pokeStrength: 0.7,
        pokeDuration: 25.0
    )

    public static let wineGlass = CharacterTemplate(
        name: "Wine Glass",
        description: "Crystalline cylindrical shell resonance",
        mode0: (1.000, 0.5, 1.00),   // Fundamental
        mode1: (2.280, 0.65, 0.65),  // First overtone
        mode2: (3.650, 0.80, 0.45),  // Second overtone
        mode3: (5.130, 1.00, 0.30),  // Third overtone
        waveShapes: (0, 0, 0, 0),    // Pure sine for glass clarity
        pokeStrength: 0.6,
        pokeDuration: 20.0
    )

    public static let freeBar = CharacterTemplate(
        name: "Free Bar",
        description: "Vibraphone/tubular chimes (widely-spaced modes)",
        mode0: (1.000, 0.5, 1.00),   // Fundamental
        mode1: (2.756, 0.60, 0.60),  // Second bending mode
        mode2: (5.404, 0.75, 0.35),  // Third bending mode
        mode3: (8.933, 0.90, 0.20),  // Fourth bending mode
        waveShapes: (0, 0, 0, 0),    // Pure sine for mallet clarity
        pokeStrength: 0.75,
        pokeDuration: 12.0
    )

    public static let tunedBar = CharacterTemplate(
        name: "Tuned Bar",
        description: "Marimba/xylophone with harmonic overtones",
        mode0: (1.000, 0.5, 1.00),   // Fundamental
        mode1: (4.000, 0.70, 0.50),  // 2 octaves (arch-tuned)
        mode2: (10.00, 1.00, 0.25),  // ~3 octaves + fifth
        mode3: (18.00, 1.25, 0.15),  // ~4 octaves
        waveShapes: (0, 0, 0, 0),    // Pure sine for wooden warmth
        pokeStrength: 0.85,
        pokeDuration: 8.0
    )

    public static let drumMembrane = CharacterTemplate(
        name: "Drum Membrane",
        description: "Timpani/kettledrum (closely-spaced modes)",
        mode0: (1.000, 0.40, 1.00),  // (0,1) fundamental
        mode1: (1.593, 0.50, 0.70),  // (1,1) mode
        mode2: (2.136, 0.60, 0.50),  // (2,1) mode
        mode3: (2.296, 0.65, 0.40),  // (0,2) mode
        waveShapes: (0, 0, 0, 0),    // Pure sine for drum tone
        pokeStrength: 0.9,
        pokeDuration: 15.0
    )

    public static let smallBell = CharacterTemplate(
        name: "Small Bell",
        description: "Handbell with stretched inharmonicity",
        mode0: (1.000, 0.30, 1.00),  // Fundamental (quick decay)
        mode1: (1.350, 0.40, 0.80),  // Stretched minor third
        mode2: (1.700, 0.50, 0.60),  // Stretched fifth
        mode3: (2.200, 0.60, 0.40),  // Upper partial
        waveShapes: (0, 0, 0, 0),    // Pure sine for bright bell
        pokeStrength: 0.85,
        pokeDuration: 6.0
    )

    public static let harmonicString = CharacterTemplate(
        name: "Harmonic String",
        description: "Perfect harmonic series (reference)",
        mode0: (1.000, 0.5, 1.00),   // Fundamental
        mode1: (2.000, 0.55, 0.60),  // Octave
        mode2: (3.000, 0.60, 0.40),  // Fifth above octave
        mode3: (4.000, 0.65, 0.30),  // Two octaves
        waveShapes: (0, 0, 0, 0),    // Pure sine for harmonic purity
        pokeStrength: 0.65,
        pokeDuration: 18.0
    )
}
