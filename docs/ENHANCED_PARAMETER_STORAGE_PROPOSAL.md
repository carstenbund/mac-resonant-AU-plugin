# Proposal: Enhanced Parameter Storage Architecture

## Current State Analysis

### Existing Storage Systems

**1. CharacterPreset (Swift - User Presets)**
```swift
struct CharacterPreset {
    // 4 modes x 3 parameters = 12 values
    var mode0Frequency: Float
    var mode0Damping: Float
    var mode0Weight: Float
    // ... modes 1-3
    var pokeStrength: Float
    var pokeDuration: Float
    var personality: Int
}
```

**2. NodeCharacter (C++ - Factory Presets)**
```cpp
struct NodeCharacter {
    float mode_freq_mult[4];
    float mode_damping[4];
    float mode_weight[4];
    node_personality_t personality;
    float poke_strength;
    float poke_duration_ms;
    float coupling_response_gain;
}
```

### Missing Capabilities

❌ **Wave shape per mode** (currently global)
❌ **Temperament offsets** (embedded in freq_mult, not editable)
❌ **Base ratio + cent offset separation** (for user editing)
❌ **Per-mode personality** (resonator vs self-osc)
❌ **Attack/release envelopes** per mode
❌ **Export/import of factory presets to user format**

---

## Proposed Solution: Three-Tier Architecture

### Tier 1: Fine-Grained DSP Parameters (C++)

**New Structure: `ModeParameters`** (per-mode control)

```cpp
/**
 * @brief Fine-grained parameters for a single modal oscillator
 */
struct ModeParameters {
    // Frequency specification (two representations)
    float freq_multiplier;      ///< Absolute multiplier (what's used in DSP)
    float base_ratio;           ///< Base harmonic ratio (1.0, 1.5, 2.0, etc.)
    float cent_offset;          ///< Temperament offset in cents (±50)

    // Timbre
    float damping;              ///< Decay rate (0.01-10.0)
    float weight;               ///< Audio contribution (0.0-1.0)
    wave_shape_t wave_shape;    ///< Oscillator waveform (0-5)

    // Envelope (per-mode attack/release)
    float attack_ms;            ///< Attack time (1.0-100.0 ms)
    float release_scale;        ///< Release time multiplier (0.5-2.0)

    // Behavior
    bool self_oscillate;        ///< Per-mode personality override
    float coupling_gain;        ///< How this mode responds to coupling
};
```

**New Structure: `NodeCharacterExtended`** (replacement for NodeCharacter)

```cpp
struct NodeCharacterExtended {
    // 4 independent modes with full control
    ModeParameters modes[4];

    // Global node behavior
    node_personality_t default_personality;
    float poke_strength;
    float poke_duration_ms;
    float coupling_response_gain;

    // Metadata
    const char* name;
    const char* description;
    const char* category;        // "Bach", "Bell", "Pad", etc.
    uint8_t version;             // For format evolution
};
```

### Tier 2: User-Editable Preset (Swift)

**Enhanced: `CharacterPresetV2`**

```swift
/// Version 2 preset format with fine-grained control
struct CharacterPresetV2: Codable, Identifiable {
    var id = UUID()
    var name: String
    var category: String = "Custom"
    var dateCreated: Date = Date()
    var version: Int = 2

    // 4 modes with full parameters
    var modes: [ModeParametersSwift]

    // Global settings
    var pokeStrength: Float
    var pokeDuration: Float
    var personality: Int
    var couplingResponseGain: Float

    // Metadata for organization
    var tags: [String] = []
    var author: String = ""
    var notes: String = ""
}

struct ModeParametersSwift: Codable {
    // Frequency (user edits base ratio + cents separately)
    var baseRatio: Float            // e.g., 1.5 for perfect fifth
    var centOffset: Float           // e.g., -2.0 for narrowed fifth

    // Computed property for DSP
    var frequencyMultiplier: Float {
        return baseRatio * pow(2.0, centOffset / 1200.0)
    }

    // Timbre
    var damping: Float
    var weight: Float
    var waveShape: Int              // 0-5

    // Envelope
    var attackMs: Float = 5.0
    var releaseScale: Float = 1.0

    // Behavior
    var selfOscillate: Bool = false
    var couplingGain: Float = 1.0
}
```

### Tier 3: Factory Preset Library (Bridging)

**Factory Preset Converter**

```swift
extension CharacterPresetV2 {
    /// Convert C++ NodeCharacter to editable preset
    static func fromFactoryCharacter(_ characterID: Int) -> CharacterPresetV2? {
        // Call C++ bridge to get character data
        guard let character = getNodeCharacter(UInt8(characterID)) else {
            return nil
        }

        // Convert to editable format
        let modes = (0..<4).map { i in
            ModeParametersSwift(
                baseRatio: character.modes[i].base_ratio,
                centOffset: character.modes[i].cent_offset,
                damping: character.modes[i].damping,
                weight: character.modes[i].weight,
                waveShape: Int(character.modes[i].wave_shape.rawValue),
                attackMs: character.modes[i].attack_ms,
                releaseScale: character.modes[i].release_scale,
                selfOscillate: character.modes[i].self_oscillate,
                couplingGain: character.modes[i].coupling_gain
            )
        }

        return CharacterPresetV2(
            name: String(cString: character.name),
            category: String(cString: character.category),
            modes: modes,
            pokeStrength: character.poke_strength,
            pokeDuration: character.poke_duration_ms,
            personality: Int(character.default_personality.rawValue),
            couplingResponseGain: character.coupling_response_gain
        )
    }

    /// Export to C++ format for DSP use
    func toNodeCharacterExtended() -> NodeCharacterExtended {
        var character = NodeCharacterExtended()

        for (i, mode) in modes.enumerated() {
            character.modes[i] = ModeParameters(
                freq_multiplier: mode.frequencyMultiplier,
                base_ratio: mode.baseRatio,
                cent_offset: mode.centOffset,
                damping: mode.damping,
                weight: mode.weight,
                wave_shape: wave_shape_t(rawValue: UInt32(mode.waveShape))!,
                attack_ms: mode.attackMs,
                release_scale: mode.releaseScale,
                self_oscillate: mode.selfOscillate,
                coupling_gain: mode.couplingGain
            )
        }

        character.poke_strength = pokeStrength
        character.poke_duration_ms = pokeDuration
        // ... etc

        return character
    }
}
```

---

## Migration Strategy

### Phase 1: Add Extended Structures (Backward Compatible)

**Week 1: C++ Layer**
1. Add `ModeParameters` and `NodeCharacterExtended` structs
2. Keep existing `NodeCharacter` for compatibility
3. Add conversion functions: `NodeCharacter` → `NodeCharacterExtended`
4. Update Bach presets to use extended format

**Week 2: Swift Layer**
1. Add `CharacterPresetV2` and `ModeParametersSwift`
2. Keep `CharacterPreset` for existing user presets
3. Add migration: `CharacterPreset` → `CharacterPresetV2`
4. Update `CharacterPresetManager` to handle both versions

### Phase 2: UI Enhancement

**Character Editor Tabs**

```
┌─────────────────────────────────────────────────┐
│ Preset: Bach-T1 Soprano              [Save] [×] │
├─────────────────────────────────────────────────┤
│ Tabs: [Mode 0] [Mode 1] [Mode 2] [Mode 3] [Global] │
├─────────────────────────────────────────────────┤
│ MODE 0 - Fundamental                            │
│                                                 │
│ Frequency:                                      │
│   Base Ratio:  [1.00] (1/1, unison)            │
│   Cent Offset: [0.0]  cents                    │
│   → Result: 1.000000                            │
│                                                 │
│ Timbre:                                         │
│   Damping: [■■■■□□□□□□] 0.40                    │
│   Weight:  [■■■■■■■■■■] 1.00                    │
│   Wave:    [Sine ▼] (Sine/Saw/Tri/Square/...)  │
│                                                 │
│ Envelope:                                       │
│   Attack:  [■■□□□□□□□□] 5.0 ms                  │
│   Release: [■■■■■□□□□□] 1.0× damping            │
│                                                 │
│ Behavior:                                       │
│   ☐ Self-Oscillate                             │
│   Coupling Gain: [■■■■■□□□□□] 1.0              │
└─────────────────────────────────────────────────┘
```

**Preset Browser Enhancement**

```
┌─────────────────────────────────────────────────┐
│ [Factory] [User] [All]                          │
├─────────────────────────────────────────────────┤
│ Category: [Bach] ▼                              │
│                                                 │
│ ♪ Bach-T1 Soprano          [Load] [Clone] [?]  │
│   Well-tempered, -2¢ fifth                      │
│                                                 │
│ ♪ Bach-T2 Alto             [Load] [Clone] [?]  │
│   Key color, +4¢ third                          │
│                                                 │
│ ♪ Bach-T3 Tenor            [Load] [Clone] [?]  │
│   Engine room, -8¢ fifth                        │
│                                                 │
│ ♪ Bach-T4 Bass             [Load] [Clone] [?]  │
│   Organ foundation, sub-octave                  │
└─────────────────────────────────────────────────┘

[Clone] = Copy factory preset to user preset for editing
[?]     = Show preset details/documentation
```

### Phase 3: Advanced Features

**Preset Interpolation**
```swift
/// Morph between two presets
func interpolate(from preset1: CharacterPresetV2,
                 to preset2: CharacterPresetV2,
                 amount: Float) -> CharacterPresetV2
```

**Preset Randomization**
```swift
/// Randomize with constraints
func randomize(basePreset: CharacterPresetV2,
               constraints: RandomizationConstraints) -> CharacterPresetV2
```

**Batch Export**
```swift
/// Export multiple presets as a preset pack
func exportPresetPack(presets: [CharacterPresetV2], to: URL) throws
```

---

## File Format

### JSON Structure (Human-Readable)

```json
{
  "version": 2,
  "name": "Bach-T1 Soprano (Custom)",
  "category": "Bach",
  "dateCreated": "2026-01-15T10:30:00Z",
  "modes": [
    {
      "baseRatio": 1.0,
      "centOffset": 0.0,
      "damping": 0.4,
      "weight": 1.0,
      "waveShape": 0,
      "attackMs": 5.0,
      "releaseScale": 1.0,
      "selfOscillate": false,
      "couplingGain": 1.0
    },
    {
      "baseRatio": 1.5,
      "centOffset": -2.0,
      "damping": 0.5,
      "weight": 0.45,
      "waveShape": 0,
      "attackMs": 5.0,
      "releaseScale": 1.0,
      "selfOscillate": false,
      "couplingGain": 1.0
    }
    // ... modes 2-3
  ],
  "pokeStrength": 0.6,
  "pokeDuration": 12.0,
  "personality": 0,
  "couplingResponseGain": 0.9,
  "tags": ["well-tempered", "soprano", "fugal"],
  "author": "System",
  "notes": "Clear subject line with living temperament offsets"
}
```

---

## Implementation Priority

### High Priority (Immediate Value)

1. ✅ **Separate base ratio + cent offset in UI**
   - Users can edit temperament without calculator
   - Shows "3/2 with -2 cents" instead of "1.498275"

2. ✅ **Per-mode wave shapes**
   - Soprano can be pure sine while tenor has sawtooth edge
   - Core feature for realizing Bach preset design intent

3. ✅ **Clone factory presets to user presets**
   - Users can customize Bach presets
   - Preserves factory presets as starting points

### Medium Priority (Enhanced Workflow)

4. **Per-mode envelopes**
   - Attack/release per mode for evolving timbre
   - Enables plucked vs bowed character differences

5. **Preset categories and tags**
   - Organize growing library
   - "Bach", "Bell", "Pad", "Drone", etc.

6. **Preset interpolation**
   - Morph between characters
   - Create variations systematically

### Low Priority (Advanced Users)

7. **Per-mode personality**
   - Mode 0 resonator, Mode 1 self-oscillator
   - Very advanced, niche use

8. **Preset randomization with constraints**
   - Generate variations
   - Stay within aesthetic bounds

---

## Storage Location

```
~/Library/Application Support/ModalAttractors/
├── Presets/
│   ├── Factory/          (Read-only, from app bundle)
│   │   ├── manifest.json
│   │   └── Bach-T1-Soprano.json
│   │   └── Bach-T2-Alto.json
│   │   └── ...
│   └── User/             (Read-write)
│       ├── My-Custom-Soprano.json
│       └── Variations/
│           └── Bach-T1-Modified.json
├── PresetPacks/          (Imported bundles)
│   └── Experimental-Pack-2026-01.bundle/
└── Settings.json
```

---

## Backward Compatibility

### Loading Old Presets

```swift
class CharacterPresetManager {
    func loadPreset(from data: Data) -> CharacterPresetV2? {
        // Try V2 format first
        if let v2 = try? JSONDecoder().decode(CharacterPresetV2.self, from: data) {
            return v2
        }

        // Fall back to V1 format
        if let v1 = try? JSONDecoder().decode(CharacterPreset.self, from: data) {
            return migrateV1toV2(v1)
        }

        return nil
    }

    private func migrateV1toV2(_ v1: CharacterPreset) -> CharacterPresetV2 {
        // Convert flat frequency values to base ratio + offset
        // Assume 0 cents offset for V1 presets
        // Default wave shapes to sine
        // ... migration logic
    }
}
```

---

## API Surface

### C++ Bridge (for Swift)

```cpp
// Get factory character in extended format
const NodeCharacterExtended* getNodeCharacterExtended(uint8_t id);

// Apply extended character to node
void applyExtendedCharacter(ModalVoice* voice,
                           const NodeCharacterExtended* character);

// Convert legacy to extended
NodeCharacterExtended convertLegacyCharacter(const NodeCharacter* legacy);

// Validate extended character
bool validateExtendedCharacter(const NodeCharacterExtended* character);
```

### Swift API

```swift
// Preset management
func loadPreset(_ id: UUID) -> CharacterPresetV2?
func savePreset(_ preset: CharacterPresetV2)
func deletePreset(_ id: UUID)
func cloneFactoryPreset(_ factoryID: Int) -> CharacterPresetV2

// Preset operations
func interpolate(from: CharacterPresetV2, to: CharacterPresetV2, amount: Float) -> CharacterPresetV2
func randomize(_ preset: CharacterPresetV2, constraints: RandomizationConstraints) -> CharacterPresetV2

// Import/Export
func exportPresetPack(_ presets: [CharacterPresetV2], to url: URL) throws
func importPresetPack(from url: URL) throws -> [CharacterPresetV2]
```

---

## Benefits Summary

### For Users

✅ **Intuitive editing**: See "3/2 with -2 cents" instead of "1.498275"
✅ **Full control**: Per-mode wave shapes, envelopes, behavior
✅ **Customization**: Clone and modify factory presets
✅ **Organization**: Categories, tags, search
✅ **Sharing**: Export/import preset packs

### For Development

✅ **Separation of concerns**: DSP layer vs UI layer vs storage
✅ **Backward compatible**: V1 presets auto-migrate
✅ **Extensible**: Easy to add new parameters
✅ **Testable**: Each tier can be tested independently
✅ **Documented**: Clear intent captured in separate fields

### For Musical Expression

✅ **Temperament-aware**: Base ratio + cents workflow
✅ **Voice-specific timbres**: Per-mode wave shapes
✅ **Evolving spectra**: Per-mode envelopes
✅ **Expressive control**: Fine-grained parameters accessible

---

## Recommendation

**Start with High Priority items 1-3:**

1. Implement `ModeParameters` and `NodeCharacterExtended` in C++
2. Implement `CharacterPresetV2` in Swift
3. Add "Clone to User Preset" button in UI
4. Update Bach presets to use extended format with wave shape recommendations

This gives immediate value (editable temperament offsets, per-mode wave shapes) while laying groundwork for future enhancements.

**Estimated effort**: 3-4 days for High Priority implementation.

Would you like me to implement Phase 1 (C++ extended structures and Swift V2 format)?
