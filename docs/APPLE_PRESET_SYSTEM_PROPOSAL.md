# Proposal: Adopt Apple's AUv3 Preset System

**Date:** January 2026
**Status:** Proposal
**Scope:** ModalAttractorsFramework/UI preset and parameter management

---

## Executive Summary

The current preset implementation uses a custom `CharacterPresetManager` class with UserDefaults storage, which operates independently from the AUv3 ecosystem. This proposal recommends migrating to Apple's built-in AUv3 preset APIs (`factoryPresets`, `currentPreset`, `userPresets`) to enable host integration, standard preset file formats, and a simplified codebase.

---

## Table of Contents

1. [Current Implementation Analysis](#1-current-implementation-analysis)
2. [Problems with Current Approach](#2-problems-with-current-approach)
3. [Apple's AUv3 Preset Mechanism](#3-apples-auv3-preset-mechanism)
4. [Proposed Implementation](#4-proposed-implementation)
5. [Migration Path](#5-migration-path)
6. [Benefits](#6-benefits)
7. [Implementation Checklist](#7-implementation-checklist)

---

## 1. Current Implementation Analysis

### 1.1 Parameter Structure

The plugin uses a well-designed hierarchical parameter tree defined in `Parameters.swift`:

| Group | Parameters | Purpose |
|-------|------------|---------|
| Global | masterGain, couplingStrength, topology, nodeCount | Overall synthesis control |
| Node Characters | node0-4Character (indexed 0-14) | Per-node template selection |
| Routing | noteRouting, multiExcite | MIDI routing behavior |
| Wave Shapes | 20 params (5 nodes × 4 modes) | Oscillator waveforms |
| Mode 0-3 | frequency, damping, weight (×4) | Modal synthesis parameters |
| Excitation | pokeStrength, pokeDuration | Attack characteristics |
| Voice | polyphony, personality | Voice behavior |

### 1.2 State Persistence

`ModalAttractorsExtensionAudioUnit.swift` implements `fullState` correctly:

```swift
public override var fullState: [String : Any]? {
    get {
        var state: [String: Any] = [:]
        state["type"] = Self.fourCharCode("aumi")
        state["subtype"] = Self.fourCharCode("Test")
        state["manufacturer"] = Self.fourCharCode("Test")
        state["version"] = 67072

        // Save all parameter values
        if let paramTree = parameterTree {
            for param in paramTree.allParameters {
                state[param.identifier] = modal_attractors_engine_get_parameter(...)
            }
        }
        return state
    }
    set { /* restore parameters */ }
}
```

### 1.3 Template System

15 built-in character templates are **hardcoded in the UI layer** (`CharacterEditorTabView.swift:310-355`):

```swift
let templates: [[(Float, Float, Float)]] = [
    // 0: Vibrant Bass
    [(1.0, 0.8, 1.0), (2.01, 1.0, 0.7), (3.02, 1.2, 0.5), (4.05, 1.5, 0.3)],
    // ... 14 more templates
]
```

### 1.4 Custom Preset System

`CharacterPresetManager.swift` provides custom preset storage:

```swift
struct CharacterPreset: Codable, Identifiable {
    var id = UUID()
    var name: String
    var dateCreated: Date
    // Mode parameters (4 modes × 3 values)
    // Excitation parameters
    // Personality
}

class CharacterPresetManager: ObservableObject {
    @Published var presets: [CharacterPreset] = []

    func savePreset(_ preset: CharacterPreset)    // → UserDefaults
    func deletePreset(id: UUID)
    func exportPresets(to url: URL)               // Not used in UI
    func importPresets(from url: URL)             // Not used in UI
}
```

---

## 2. Problems with Current Approach

### 2.1 Siloed Preset Systems

| System | Storage | Visibility |
|--------|---------|------------|
| AU State | Host-managed (DAW project) | DAW only |
| Custom Presets | UserDefaults | App only |
| Built-in Templates | Hardcoded in UI | UI only |

Users cannot:
- Browse presets from Logic Pro's preset menu
- Share `.aupreset` files
- Sync presets across devices via iCloud
- See factory presets in host applications

### 2.2 Architectural Issues

1. **Template Data in View Layer**: Character templates belong in the audio unit, not `CharacterEditorTabView`
2. **Duplicate State Capture**: `CharacterPreset` manually captures what `fullState` already handles
3. **No Factory Presets**: The 15 built-in templates aren't exposed via `factoryPresets`
4. **No Current Preset Tracking**: Missing `currentPreset` implementation
5. **Incompatible with Host Preset Management**: DAWs can't discover or manage presets

### 2.3 Code Duplication

The same template values appear in multiple places:
- `Parameters.swift` - Node character names as `valueStrings`
- `CharacterEditorTabView.swift` - Template data arrays
- `NodeCharacter.cpp` - C++ DSP character definitions

---

## 3. Apple's AUv3 Preset Mechanism

### 3.1 Core APIs

Apple provides these properties in `AUAudioUnit`:

```swift
// MARK: - Factory Presets (Read-only, provided by developer)
var factoryPresets: [AUAudioUnitPreset]? { get }

// MARK: - Current Preset (nil = custom/manual state)
var currentPreset: AUAudioUnitPreset? { get set }

// MARK: - User Presets (macOS 10.15+ / iOS 13+)
var userPresets: [AUAudioUnitPreset] { get }
func saveUserPreset(_ preset: AUAudioUnitPreset) throws
func deleteUserPreset(_ preset: AUAudioUnitPreset) throws
func presetState(for preset: AUAudioUnitPreset) throws -> [String: Any]

// MARK: - State (Already implemented)
var fullState: [String: Any]?
var fullStateForDocument: [String: Any]?
```

### 3.2 AUAudioUnitPreset Structure

```swift
open class AUAudioUnitPreset: NSObject, NSSecureCoding {
    open var number: Int      // Factory: 0+ | User: negative
    open var name: String
}
```

- **Factory presets**: `number >= 0`, provided by plugin developer
- **User presets**: `number < 0`, created/managed by user

### 3.3 Preset State Flow

```
┌─────────────┐     currentPreset = X     ┌──────────────────┐
│   Host/UI   │ ────────────────────────▶ │   Audio Unit     │
└─────────────┘                           └────────┬─────────┘
                                                   │
                                          presetState(for: X)
                                                   │
                                                   ▼
                                          ┌──────────────────┐
                                          │   Apply State    │
                                          │   to Parameters  │
                                          └──────────────────┘
```

---

## 4. Proposed Implementation

### 4.1 Factory Preset Data Structure

Create a new file `FactoryPresets.swift` in the audio unit layer:

```swift
// ModalAttractorsExtension/Presets/FactoryPresets.swift

import AudioToolbox

/// Factory preset definition containing all parameter values
struct FactoryPresetData {
    let name: String

    // Mode parameters (4 modes)
    let mode0: (frequency: Float, damping: Float, weight: Float)
    let mode1: (frequency: Float, damping: Float, weight: Float)
    let mode2: (frequency: Float, damping: Float, weight: Float)
    let mode3: (frequency: Float, damping: Float, weight: Float)

    // Excitation
    let pokeStrength: Float
    let pokeDuration: Float

    // Voice
    let personality: Int

    /// Convert to state dictionary for fullState
    func toStateDictionary() -> [String: Any] {
        return [
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
            "pokeStrength": pokeStrength,
            "pokeDuration": pokeDuration,
            "personality": Float(personality)
        ]
    }
}

/// All factory presets (matching NodeCharacter.cpp)
let ModalAttractorsFactoryPresets: [FactoryPresetData] = [
    // 0: Vibrant Bass
    FactoryPresetData(
        name: "Vibrant Bass",
        mode0: (1.0, 0.8, 1.0),
        mode1: (2.01, 1.0, 0.7),
        mode2: (3.02, 1.2, 0.5),
        mode3: (4.05, 1.5, 0.3),
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
        pokeStrength: 0.7,
        pokeDuration: 35.0,
        personality: 0
    )
]
```

### 4.2 Audio Unit Preset Implementation

Add to `ModalAttractorsExtensionAudioUnit.swift`:

```swift
// MARK: - Preset Support

/// Cached factory preset objects
private lazy var _factoryPresets: [AUAudioUnitPreset] = {
    return ModalAttractorsFactoryPresets.enumerated().map { index, data in
        AUAudioUnitPreset(number: index, name: data.name)
    }
}()

/// Currently selected preset (nil = custom state)
private var _currentPreset: AUAudioUnitPreset?

// MARK: - Factory Presets

public override var factoryPresets: [AUAudioUnitPreset]? {
    return _factoryPresets
}

// MARK: - Current Preset

public override var currentPreset: AUAudioUnitPreset? {
    get { return _currentPreset }
    set {
        guard let preset = newValue else {
            _currentPreset = nil
            return
        }

        // Factory preset (number >= 0)
        if preset.number >= 0 && preset.number < ModalAttractorsFactoryPresets.count {
            applyFactoryPreset(at: preset.number)
            _currentPreset = preset
        }
        // User preset (number < 0) - handled by userPresets API
        else if preset.number < 0 {
            if let state = try? presetState(for: preset) {
                fullState = state
                _currentPreset = preset
            }
        }
    }
}

/// Apply factory preset values to parameters
private func applyFactoryPreset(at index: Int) {
    guard index >= 0 && index < ModalAttractorsFactoryPresets.count,
          let paramTree = parameterTree,
          let engine = engine else { return }

    let presetData = ModalAttractorsFactoryPresets[index]
    let state = presetData.toStateDictionary()

    // Apply each parameter from the preset
    for (identifier, value) in state {
        if let param = paramTree.parameter(withIdentifier: identifier),
           let floatValue = value as? Float {
            param.value = floatValue
            modal_attractors_engine_set_parameter(engine, UInt32(param.address), floatValue)
        }
    }
}

// MARK: - User Presets (macOS 10.15+ / iOS 13+)

@available(macOS 10.15, iOS 13.0, *)
public override var supportsUserPresets: Bool {
    return true
}

// Note: saveUserPreset, deleteUserPreset, and presetState(for:)
// have default implementations that use the file system.
// Override only if custom behavior is needed.

// MARK: - State with Preset Info

public override var fullState: [String : Any]? {
    get {
        var state = super.fullState ?? [:]

        // Add component identification
        state["type"] = Self.fourCharCode("aumi")
        state["subtype"] = Self.fourCharCode("Test")
        state["manufacturer"] = Self.fourCharCode("Test")
        state["version"] = 67072

        // Include current preset info if set
        if let preset = _currentPreset {
            state["presetNumber"] = preset.number
            state["presetName"] = preset.name
        }

        // Save all parameter values
        if let paramTree = parameterTree, let engine = engine {
            for param in paramTree.allParameters {
                let value = modal_attractors_engine_get_parameter(engine, UInt32(param.address))
                state[param.identifier] = value
            }
        }

        return state
    }
    set {
        guard let newState = newValue else { return }

        // Restore preset reference if present
        if let presetNumber = newState["presetNumber"] as? Int,
           let presetName = newState["presetName"] as? String {
            _currentPreset = AUAudioUnitPreset(number: presetNumber, name: presetName)
        } else {
            _currentPreset = nil
        }

        // Restore parameter values
        if let paramTree = parameterTree, let engine = engine {
            for param in paramTree.allParameters {
                if let value = newState[param.identifier] as? Float {
                    param.value = value
                    modal_attractors_engine_set_parameter(engine, UInt32(param.address), value)
                }
            }
        }
    }
}
```

### 4.3 Updated UI Integration

Modify `CharacterEditorTabView.swift` to use AU presets:

```swift
// CharacterEditorTabView.swift

struct CharacterEditorTabView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    // Access to audio unit for preset operations
    var audioUnit: ModalAttractorsExtensionAudioUnit?

    @State private var selectedNodeIndex: Int = 0
    @State private var selectedPresetIndex: Int = 0

    // Get factory presets from audio unit
    private var factoryPresets: [AUAudioUnitPreset] {
        return audioUnit?.factoryPresets ?? []
    }

    // Template section using AU factory presets
    private var templateSection: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            Text("PRESETS")
                .font(UIConstants.Fonts.sectionTitle)

            // Factory presets from AU
            Picker("Preset", selection: $selectedPresetIndex) {
                ForEach(0..<factoryPresets.count, id: \.self) { index in
                    Text(factoryPresets[index].name).tag(index)
                }
            }
            .pickerStyle(.menu)

            Button("Load Preset") {
                loadPreset(at: selectedPresetIndex)
            }
            .buttonStyle(.bordered)

            // User presets (if available)
            if #available(macOS 10.15, *) {
                userPresetsSection
            }
        }
    }

    private func loadPreset(at index: Int) {
        guard index < factoryPresets.count else { return }
        audioUnit?.currentPreset = factoryPresets[index]
    }

    @available(macOS 10.15, *)
    private var userPresetsSection: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.small) {
            let userPresets = audioUnit?.userPresets ?? []

            if !userPresets.isEmpty {
                Divider()
                Text("User Presets (\(userPresets.count))")
                    .font(.caption)

                ForEach(userPresets, id: \.number) { preset in
                    Button(preset.name) {
                        audioUnit?.currentPreset = preset
                    }
                }
            }

            Button("Save Current as User Preset") {
                saveUserPreset()
            }
            .buttonStyle(.bordered)
        }
    }

    @available(macOS 10.15, *)
    private func saveUserPreset() {
        // Show naming dialog, then:
        let preset = AUAudioUnitPreset(number: -1, name: "My Preset")
        try? audioUnit?.saveUserPreset(preset)
    }
}
```

---

## 5. Migration Path

### Phase 1: Add Factory Presets (Non-Breaking)

1. Create `FactoryPresets.swift` with preset data
2. Implement `factoryPresets` property
3. Implement `currentPreset` get/set
4. Update `fullState` to include preset info

**Result:** Factory presets appear in host preset menus. Existing functionality unchanged.

### Phase 2: Update UI (Parallel Systems)

1. Add preset picker using `factoryPresets` to UI
2. Keep `CharacterPresetManager` for backwards compatibility
3. Test host integration (Logic Pro, GarageBand, AUM)

**Result:** Both systems work side-by-side during transition.

### Phase 3: Migrate User Presets (Optional)

1. Implement `supportsUserPresets = true`
2. Add UI for saving/deleting user presets via AU API
3. Offer one-time migration of UserDefaults presets to AU user presets
4. Deprecate `CharacterPresetManager`

**Result:** Full Apple preset integration.

### Phase 4: Cleanup

1. Remove `CharacterPresetManager.swift`
2. Remove template data from `CharacterEditorTabView.swift`
3. Update documentation

---

## 6. Benefits

| Feature | Before | After |
|---------|--------|-------|
| **Host Preset Menu** | Not visible | All presets in Logic/GarageBand menu |
| **Preset Files** | Custom UserDefaults | Standard `.aupreset` format |
| **Preset Sharing** | Export/import methods (unused) | Drag & drop `.aupreset` files |
| **iCloud Sync** | None | Automatic (when supported) |
| **A/B Comparison** | Manual | Host-native support |
| **Code Complexity** | 200+ lines in CharacterPresetManager | ~50 lines in AU |
| **State Consistency** | Dual systems | Single source of truth |

### Host Integration Examples

**Logic Pro X:**
- Factory presets appear in the preset dropdown
- User presets saved via Logic's preset browser
- Undo/redo tracks preset changes

**GarageBand:**
- Presets in instrument settings
- Quick preset switching

**AUM (iOS):**
- Preset browser integration
- State save/restore with session

---

## 7. Implementation Checklist

### Audio Unit Layer

- [ ] Create `ModalAttractorsExtension/Presets/FactoryPresets.swift`
- [ ] Define `FactoryPresetData` struct
- [ ] Populate `ModalAttractorsFactoryPresets` array with 15 presets
- [ ] Add `_factoryPresets` lazy property
- [ ] Add `_currentPreset` property
- [ ] Override `factoryPresets` getter
- [ ] Override `currentPreset` get/set
- [ ] Implement `applyFactoryPreset(at:)`
- [ ] Add `supportsUserPresets` (macOS 10.15+)
- [ ] Update `fullState` to include preset info

### UI Layer

- [ ] Pass `audioUnit` reference to `CharacterEditorTabView`
- [ ] Replace hardcoded templates with `factoryPresets`
- [ ] Add user preset UI (save/delete)
- [ ] Update `PresetBrowserView` to show both factory and user presets

### Migration

- [ ] Test factory presets in Logic Pro
- [ ] Test factory presets in GarageBand
- [ ] Test user preset save/load
- [ ] Migrate existing UserDefaults presets (optional)
- [ ] Remove `CharacterPresetManager` after migration

### Documentation

- [ ] Update user documentation
- [ ] Add preset usage examples
- [ ] Document `.aupreset` file location

---

## Appendix A: File Locations

| File | Purpose |
|------|---------|
| `ModalAttractorsExtension/Presets/FactoryPresets.swift` | New: Factory preset data |
| `ModalAttractorsFramework/Common/Audio Unit/ModalAttractorsExtensionAudioUnit.swift` | Modify: Add preset APIs |
| `ModalAttractorsFramework/UI/CharacterEditorTabView.swift` | Modify: Use AU presets |
| `ModalAttractorsFramework/UI/Utilities/CharacterPresetManager.swift` | Deprecate: Remove after migration |

## Appendix B: User Preset File Location

Apple stores user presets at:

**macOS:**
```
~/Library/Audio/Presets/<Manufacturer>/<AudioUnit>/
```

For this plugin:
```
~/Library/Audio/Presets/Test/ModalAttractors/
```

**iOS:**
```
<AppContainer>/Library/Audio/Presets/<Manufacturer>/<AudioUnit>/
```

## Appendix C: References

- [AUAudioUnit Class Reference](https://developer.apple.com/documentation/audiotoolbox/auaudiounit)
- [Audio Unit Hosting Guide](https://developer.apple.com/documentation/audiotoolbox/audio_unit_hosting_guide)
- [WWDC 2019: What's New in AVAudioEngine](https://developer.apple.com/videos/play/wwdc2019/510/)
