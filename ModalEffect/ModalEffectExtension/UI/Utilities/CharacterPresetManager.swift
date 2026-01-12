//
//  CharacterPresetManager.swift
//  ModalEffectExtension
//
//  Manages saving and loading of custom character presets
//

import Foundation

/// Represents a complete character preset with all parameters
struct CharacterPreset: Codable, Identifiable {
    var id = UUID()
    var name: String
    var dateCreated: Date = Date()

    // Mode parameters (4 modes x 3 params each)
    var mode0Frequency: Float
    var mode0Damping: Float
    var mode0Weight: Float

    var mode1Frequency: Float
    var mode1Damping: Float
    var mode1Weight: Float

    var mode2Frequency: Float
    var mode2Damping: Float
    var mode2Weight: Float

    var mode3Frequency: Float
    var mode3Damping: Float
    var mode3Weight: Float

    // Excitation parameters
    var pokeStrength: Float
    var pokeDuration: Float

    // Personality
    var personality: Int

    /// Create a preset from current parameter tree values
    init(name: String, from parameterTree: ParameterTree) {
        self.name = name

        // Mode 0
        self.mode0Frequency = parameterTree.mode0.frequency.value
        self.mode0Damping = parameterTree.mode0.damping.value
        self.mode0Weight = parameterTree.mode0.weight.value

        // Mode 1
        self.mode1Frequency = parameterTree.mode1.frequency.value
        self.mode1Damping = parameterTree.mode1.damping.value
        self.mode1Weight = parameterTree.mode1.weight.value

        // Mode 2
        self.mode2Frequency = parameterTree.mode2.frequency.value
        self.mode2Damping = parameterTree.mode2.damping.value
        self.mode2Weight = parameterTree.mode2.weight.value

        // Mode 3
        self.mode3Frequency = parameterTree.mode3.frequency.value
        self.mode3Damping = parameterTree.mode3.damping.value
        self.mode3Weight = parameterTree.mode3.weight.value

        // Excitation
        self.pokeStrength = parameterTree.excitation.pokeStrength.value
        self.pokeDuration = parameterTree.excitation.pokeDuration.value

        // Personality
        self.personality = Int(parameterTree.voice.personality.value)
    }

    /// Apply this preset to a parameter tree
    func apply(to parameterTree: ParameterTree) {
        // Mode 0
        parameterTree.mode0.frequency.value = mode0Frequency
        parameterTree.mode0.damping.value = mode0Damping
        parameterTree.mode0.weight.value = mode0Weight

        // Mode 1
        parameterTree.mode1.frequency.value = mode1Frequency
        parameterTree.mode1.damping.value = mode1Damping
        parameterTree.mode1.weight.value = mode1Weight

        // Mode 2
        parameterTree.mode2.frequency.value = mode2Frequency
        parameterTree.mode2.damping.value = mode2Damping
        parameterTree.mode2.weight.value = mode2Weight

        // Mode 3
        parameterTree.mode3.frequency.value = mode3Frequency
        parameterTree.mode3.damping.value = mode3Damping
        parameterTree.mode3.weight.value = mode3Weight

        // Excitation
        parameterTree.excitation.pokeStrength.value = pokeStrength
        parameterTree.excitation.pokeDuration.value = pokeDuration

        // Personality
        parameterTree.voice.personality.value = Float(personality)
    }
}

/// Manages persistence of custom character presets
class CharacterPresetManager: ObservableObject {
    static let shared = CharacterPresetManager()

    @Published private(set) var presets: [CharacterPreset] = []

    private let userDefaultsKey = "ModalEffect.CustomCharacterPresets"

    private init() {
        loadPresets()
    }

    // MARK: - Preset Management

    /// Save a new preset
    func savePreset(_ preset: CharacterPreset) {
        presets.append(preset)
        persistPresets()
    }

    /// Delete a preset
    func deletePreset(at index: Int) {
        guard index < presets.count else { return }
        presets.remove(at: index)
        persistPresets()
    }

    /// Delete a preset by ID
    func deletePreset(id: UUID) {
        presets.removeAll { $0.id == id }
        persistPresets()
    }

    /// Update an existing preset
    func updatePreset(_ preset: CharacterPreset) {
        if let index = presets.firstIndex(where: { $0.id == preset.id }) {
            presets[index] = preset
            persistPresets()
        }
    }

    // MARK: - Persistence

    /// Load presets from UserDefaults
    private func loadPresets() {
        guard let data = UserDefaults.standard.data(forKey: userDefaultsKey) else {
            presets = []
            return
        }

        do {
            let decoder = JSONDecoder()
            presets = try decoder.decode([CharacterPreset].self, from: data)
        } catch {
            print("Failed to load presets: \(error)")
            presets = []
        }
    }

    /// Save presets to UserDefaults
    private func persistPresets() {
        do {
            let encoder = JSONEncoder()
            encoder.outputFormatting = .prettyPrinted
            let data = try encoder.encode(presets)
            UserDefaults.standard.set(data, forKey: userDefaultsKey)
        } catch {
            print("Failed to save presets: \(error)")
        }
    }

    // MARK: - Export/Import (Future Enhancement)

    /// Export presets to file
    func exportPresets(to url: URL) throws {
        let encoder = JSONEncoder()
        encoder.outputFormatting = .prettyPrinted
        let data = try encoder.encode(presets)
        try data.write(to: url)
    }

    /// Import presets from file
    func importPresets(from url: URL) throws {
        let data = try Data(contentsOf: url)
        let decoder = JSONDecoder()
        let imported = try decoder.decode([CharacterPreset].self, from: data)

        // Merge with existing presets (avoid duplicates by name)
        for preset in imported {
            if !presets.contains(where: { $0.name == preset.name }) {
                presets.append(preset)
            }
        }

        persistPresets()
    }
}
