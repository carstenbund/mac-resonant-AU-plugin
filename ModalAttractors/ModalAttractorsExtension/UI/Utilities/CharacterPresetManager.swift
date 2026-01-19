//
//  CharacterPresetManager.swift
//  ModalAttractorsExtension
//
//  Manages saving and loading of custom character presets
//  Now using ParameterStore for clean separation of concerns
//

import Foundation
import Combine

/// Represents a complete character preset with all parameters
/// Uses dictionary-based storage compatible with Logic's fullState system
struct CharacterPreset: Codable, Identifiable {
    var id = UUID()
    var name: String
    var dateCreated: Date = Date()

    // Parameter storage using dictionary format (compatible with fullState)
    private var parameters: [String: Float]

    /// Create a preset from a parameter store
    /// - Parameters:
    ///   - name: Name of the preset
    ///   - store: Parameter store to read values from
    init(name: String, from store: ParameterStore) {
        self.name = name
        self.parameters = store.values
    }

    /// Create a preset from current parameter tree values
    /// - Parameters:
    ///   - name: Name of the preset
    ///   - parameterTree: Parameter tree to read values from
    ///   - nodeIndex: Node index to read wave shapes from (0-4)
    init(name: String, from parameterTree: ParameterTree, nodeIndex: Int) {
        self.name = name

        // Create a parameter store and load from parameter tree
        let store = ParameterStore()
        store.loadFrom(parameterTree)

        // Store only the character-related parameters (not global/network settings)
        self.parameters = store.values.filter { key, _ in
            key.hasPrefix("mode") ||
            key.hasPrefix("node\(nodeIndex)Mode") ||
            key == "pokeStrength" ||
            key == "pokeDuration" ||
            key == "personality"
        }
    }

    /// Apply this preset to a parameter store
    /// - Parameters:
    ///   - store: Parameter store to apply values to
    ///   - nodeIndex: Node index to apply wave shapes to (0-4)
    func apply(to store: ParameterStore, nodeIndex: Int) {
        // Apply all stored parameters
        for (key, value) in parameters {
            // Handle wave shapes - remap to target node index
            if key.contains("Mode") && key.contains("WaveShape") {
                // Extract mode index from key like "node0Mode2WaveShape"
                if let modeStr = key.split(separator: "Mode").last?.prefix(1),
                   let modeIndex = Int(modeStr) {
                    store.setWaveShape(nodeIndex: nodeIndex, modeIndex: modeIndex, waveShape: Int(value))
                }
            } else {
                // All other parameters apply directly
                store.setValue(key, value: value)
            }
        }
    }

    /// Apply this preset to a parameter tree
    /// - Parameters:
    ///   - parameterTree: Parameter tree to apply values to
    ///   - nodeIndex: Node index to apply wave shapes to (0-4)
    func apply(to parameterTree: ParameterTree, nodeIndex: Int) {
        // Create a temporary store, apply values, then apply to parameter tree
        let store = ParameterStore()
        apply(to: store, nodeIndex: nodeIndex)
        store.applyTo(parameterTree, nodeIndex: nodeIndex)
    }

    /// Get a parameter store from this preset
    /// - Parameter nodeIndex: Node index for wave shapes
    /// - Returns: Parameter store with preset values
    func toParameterStore(nodeIndex: Int) -> ParameterStore {
        let store = ParameterStore()
        apply(to: store, nodeIndex: nodeIndex)
        return store
    }
}

/// Manages persistence of custom character presets
class CharacterPresetManager: ObservableObject {
    static let shared = CharacterPresetManager()

    @Published private(set) var presets: [CharacterPreset] = []

    private let userDefaultsKey = "ModalAttractors.CustomCharacterPresets"

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
