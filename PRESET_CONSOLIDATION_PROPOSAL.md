# Preset System Consolidation Proposal
**Modal Attractors Plugin - Development Stage**

## Executive Summary

This proposal consolidates two parallel preset systems and eliminates the legacy 3-value-per-mode structure, unifying all character/preset definitions under the C++ `NodeCharacter` system with a modern class-based API.

### Current State
- **Two preset systems exist in parallel:**
  - `MODAL_PRESETS[]` in `modal_node.c` (8 physically-derived presets, C-only, not UI-integrated)
  - `CHARACTER_LIBRARY[]` in `NodeCharacter.cpp` (15 characters, fully UI-integrated)
  - `CharacterTemplates.swift` mirrors both (23 total templates)

- **Legacy 3-value structure still active:**
  - `mode_preset_t { freq_ratio, damping_ratio, weight }` in `modal_node.c`
  - Limited representation (missing wave shapes, personality, excitation params)
  - No longer suitable for the expanded parameter system

- **Duplication detected:**
  - `BRIGHT_BELL` character ≈ `Free Bar` preset (freq ratios: 1.0, 2.76, 5.40, 8.93)
  - Both systems describe the same sonic concepts with different names

### Proposed Solution
**One unified system:** `NodeCharacter` array-based library with `CharacterManager` class

---

## Current Architecture Analysis

### 1. DSP Layer (C/C++)

#### A. Legacy System: modal_node.c
```c
// Location: ModalAttractors/ModalAttractorsExtension/DSP/modal_node.c:107-211

typedef struct {
    float freq_ratio;     // Frequency relative to fundamental
    float damping_ratio;  // Relative damping
    float weight;         // Audio weight [0,1]
} mode_preset_t;

typedef struct {
    const char* name;
    const char* description;
    mode_preset_t modes[4];  // 3 values × 4 modes = 12 values total
} modal_preset_t;

static const modal_preset_t MODAL_PRESETS[] = {
    // 8 physically-derived presets
    // Church Bell, Circular Plate, Wine Glass, Free Bar, Tuned Bar,
    // Drum Membrane, Small Bell, Harmonic String
};
```

**API Functions:**
- `modal_node_apply_preset(node, preset_id, fundamental_hz, damping)`
- `modal_node_get_preset_name(preset_id)`
- `modal_node_get_num_presets()`

**Limitations:**
- ❌ Only 3 values per mode (freq_ratio, damping_ratio, weight)
- ❌ No wave shape support
- ❌ No personality/excitation parameters
- ❌ Not integrated with UI
- ❌ C-style API, not used by main engine

#### B. Current System: NodeCharacter.cpp
```cpp
// Location: ModalAttractors/ModalAttractorsExtension/DSP/NodeCharacter.{h,cpp}

struct NodeCharacter {
    float mode_freq_mult[4];        // Frequency multipliers (same as freq_ratio)
    float mode_damping[4];          // Damping coefficients (same as damping_ratio)
    float mode_weight[4];           // Audio weights (same as weight)
    wave_shape_t mode_shape[4];     // Wave shapes per mode ✓ NEW
    node_personality_t personality; // Resonator vs Self-Oscillator ✓ NEW
    float poke_strength;            // Excitation strength ✓ NEW
    float poke_duration_ms;         // Excitation duration ✓ NEW
    float coupling_response_gain;   // Network coupling behavior ✓ NEW
    const char* name;               // Display name
    const char* description;        // Short description
};

// 15 individual constants defined: CHARACTER_VIBRANT_BASS, CHARACTER_DARK_NODE, etc.
const NodeCharacter* CHARACTER_LIBRARY[15] = { &CHARACTER_VIBRANT_BASS, ... };
```

**API Functions:**
- `getCharacter(character_id)` → returns `NodeCharacter*`
- `getCharacterName(character_id)` → returns name string
- `validateCharacter(character)` → validates parameters

**Advantages:**
- ✅ Rich parameter set (11 fields)
- ✅ Fully integrated with UI
- ✅ Used by `NodeManager::setNodeCharacter()`
- ✅ Validation support
- ✅ Metadata included

### 2. UI Layer (Swift)

#### CharacterTemplates.swift
```swift
// Location: ModalAttractorsFramework/UI/Utilities/CharacterTemplates.swift

struct CharacterTemplate {
    let name: String
    let description: String
    let mode0: (frequency: Float, damping: Float, weight: Float)
    let mode1: (frequency: Float, damping: Float, weight: Float)
    let mode2: (frequency: Float, damping: Float, weight: Float)
    let mode3: (frequency: Float, damping: Float, weight: Float)
    let waveShapes: (mode0: Int, mode1: Int, mode2: Int, mode3: Int)
    let pokeStrength: Float
    let pokeDuration: Float
    let personality: Int

    func apply(to store: ParameterStore, nodeIndex: Int)
}

static let all: [CharacterTemplate] = [
    // 15 original characters (mirroring NodeCharacter.cpp)
    vibrantBass, darkNode, brightBell, glassyShimmer, droneHub,
    metallicStrike, warmPad, percussiveHit, resonantBell, deepRumble,
    harmonicStack, detunedChorus, malletTone, windChime, gongWash,

    // 8 physically-derived presets (manually added from modal_node.c)
    churchBell, circularPlate, wineGlass, freeBar, tunedBar,
    drumMembrane, smallBell, harmonicString
]
```

**Status:**
- Swift manually mirrors C++ definitions
- 23 total templates (15 + 8)
- Already duplicates work between C++ and Swift
- The 8 physical presets exist only in Swift, not in active C++ CHARACTER_LIBRARY

#### CharacterPresetManager.swift
```swift
struct CharacterPreset: Codable {
    var name: String
    var dateCreated: Date
    private var parameters: [String: Float]  // Dictionary-based storage

    init(name: String, from store: ParameterStore)
    func apply(to store: ParameterStore, nodeIndex: Int)
}

class CharacterPresetManager {
    func savePreset(_ preset: CharacterPreset)
    func loadPreset(id: UUID) -> CharacterPreset?
    func deletePreset(id: UUID)
}
```

**Status:**
- Uses dictionary-based storage (compatible with `AUAudioUnit.fullState`)
- Stores user-created presets (UserDefaults + JSON export)
- Works with ParameterStore for clean separation

### 3. Parameter System

**Data Flow: UI → Parameters → DSP**

```
NodeCharactersView (dropdown: 0-14)
    ↓
ParameterTree.nodeCharacters.node0-4 (AUParameter)
    ↓
SynthEngine::setParameter(kParam_Node0_Character, characterID)
    ↓
NodeManager::setNodeCharacter(nodeIdx, characterID)
    ↓
CHARACTER_LIBRARY[characterID] → applyCharacterToNode()
    ↓
ModalVoice::setPersonality(), modal_node_set_mode()
```

**Current Parameter Addresses:**
- `param_Node0_Character` (address 20) through `param_Node4_Character` (address 24)
- `param_Mode0_Frequency` (address 4), `param_Mode0_Damping` (5), `param_Mode0_Weight` (6)
- Wave shapes: `param_Node0_Mode0_WaveShape` (address 27) through `param_Node4_Mode3_WaveShape` (address 46)

---

## Detected Overlaps & Duplications

### Character vs Preset Comparison

| NodeCharacter (C++) | MODAL_PRESETS (C) | Freq Ratios | Status |
|---------------------|-------------------|-------------|---------|
| `CHARACTER_BRIGHT_BELL` | `Free Bar` | 1.0, 2.76, 5.40, 8.93 | **DUPLICATE** ⚠️ |
| `CHARACTER_HARMONIC_STACK` | `Harmonic String` | 1.0, 2.0, 3.0, 4.0 | **DUPLICATE** ⚠️ |
| `CHARACTER_WARM_PAD` | `Harmonic String` | 1.0, 2.0, 3.0, 4.0 | **DUPLICATE** ⚠️ |
| `CHARACTER_RESONANT_BELL` | - | 1.0, 2.0, 3.0, 4.0 | Harmonic variant |
| - | `Church Bell` | 1.0, 1.19, 1.5, 2.0 | **MISSING in C++** |
| - | `Circular Plate` | 1.0, 2.08, 3.41, 3.89 | **MISSING in C++** |
| - | `Wine Glass` | 1.0, 2.28, 3.65, 5.13 | **MISSING in C++** |
| - | `Tuned Bar` | 1.0, 4.0, 10.0, 18.0 | **MISSING in C++** |
| - | `Drum Membrane` | 1.0, 1.59, 2.14, 2.30 | **MISSING in C++** |
| - | `Small Bell` | 1.0, 1.35, 1.7, 2.2 | **MISSING in C++** |

**Findings:**
- 3 direct duplicates (Bright Bell, Harmonic Stack, Warm Pad)
- 6 physically-derived presets exist only in C but not in C++ CHARACTER_LIBRARY
- Swift CharacterTemplates.swift has all 23 (manually maintaining both systems)

---

## Proposed Consolidation Strategy

### Phase 1: Unify Data Structures (C++ Side)

#### 1.1 Expand CHARACTER_LIBRARY with Physical Presets

**File:** `ModalAttractors/ModalAttractorsExtension/DSP/NodeCharacter.cpp`

**Action:** Add 6 missing physical presets to CHARACTER_LIBRARY as NodeCharacter entries

```cpp
// Add after existing characters:

const NodeCharacter CHARACTER_CHURCH_BELL = {
    .mode_freq_mult = {1.0f, 1.19f, 1.5f, 2.0f},
    .mode_damping = {1.0f, 1.2f, 1.4f, 1.8f},
    .mode_weight = {1.0f, 0.75f, 0.6f, 0.45f},
    .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.7f,
    .poke_duration_ms = 10.0f,
    .coupling_response_gain = 1.0f,
    .name = "Church Bell",
    .description = "Western church bell with hum, fundamental, tierce, and quint"
};

const NodeCharacter CHARACTER_CIRCULAR_PLATE = {
    .mode_freq_mult = {1.0f, 2.081f, 3.413f, 3.891f},
    .mode_damping = {1.0f, 1.1f, 1.3f, 1.4f},
    .mode_weight = {1.0f, 0.7f, 0.5f, 0.35f},
    .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.8f,
    .poke_duration_ms = 8.0f,
    .coupling_response_gain = 1.0f,
    .name = "Circular Plate",
    .description = "Flat circular plate (cymbal, gong) - Rayleigh modes"
};

// ... Wine Glass, Tuned Bar, Drum Membrane, Small Bell (6 total new entries)

// Update NUM_BUILTIN_CHARACTERS from 15 to 21 in NodeCharacter.h
#define NUM_BUILTIN_CHARACTERS 21
```

**Result:** 21 total characters (15 original + 6 new physical)

#### 1.2 Remove Duplicates

**Action:** Decide canonical names and remove duplicates

**Decision Matrix:**
| Keep | Remove | Reason |
|------|--------|--------|
| `CHARACTER_BRIGHT_BELL` | ~~Free Bar~~ | More descriptive name |
| `CHARACTER_HARMONIC_STACK` | ~~Harmonic String~~ | Already in use |
| `CHARACTER_WARM_PAD` | Keep separate | Different excitation params (poke_duration: 30ms vs 15ms) |

**Final count:** 19 unique characters

#### 1.3 Convert to Array-Driven Structure

**Current (scattered constants):**
```cpp
const NodeCharacter CHARACTER_VIBRANT_BASS = { ... };
const NodeCharacter CHARACTER_DARK_NODE = { ... };
// ... 15 individual constants

const NodeCharacter* CHARACTER_LIBRARY[NUM_BUILTIN_CHARACTERS] = {
    &CHARACTER_VIBRANT_BASS, &CHARACTER_DARK_NODE, ...
};
```

**Proposed (array-driven):**
```cpp
// In NodeCharacter.cpp
static const NodeCharacter CHARACTER_LIBRARY[] = {
    // Original characters (0-14)
    {
        .mode_freq_mult = {1.0f, 2.0f, 3.0f, 5.0f},
        .mode_damping = {0.3f, 0.5f, 0.8f, 1.2f},
        .mode_weight = {1.0f, 0.8f, 0.6f, 0.4f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.7f,
        .poke_duration_ms = 15.0f,
        .coupling_response_gain = 0.8f,
        .name = "Vibrant Bass",
        .description = "Strong harmonic bass with sustained low end"
    },
    // ... all 19 characters in array
};

#define NUM_BUILTIN_CHARACTERS (sizeof(CHARACTER_LIBRARY) / sizeof(NodeCharacter))
```

**Benefits:**
- ✅ Single source of truth
- ✅ Automatic count calculation
- ✅ Easier to add new characters
- ✅ No scattered constant names to maintain

### Phase 2: Create CharacterManager Class

**File:** `ModalAttractors/ModalAttractorsExtension/DSP/CharacterManager.{h,cpp}` (NEW)

```cpp
// CharacterManager.h

#ifndef CHARACTER_MANAGER_H
#define CHARACTER_MANAGER_H

#include "NodeCharacter.h"
#include <string>
#include <optional>
#include <cstdint>

/**
 * @brief Manages character library access and validation
 *
 * Provides unified API for character retrieval, validation,
 * and optional user preset management.
 */
class CharacterManager {
public:
    /**
     * @brief Get character by index
     * @param index Character ID (0 to getCharacterCount()-1)
     * @return Const reference to character, or default character if invalid
     */
    static const NodeCharacter& getCharacter(size_t index);

    /**
     * @brief Get total number of built-in characters
     * @return Character count
     */
    static size_t getCharacterCount();

    /**
     * @brief Find character index by name
     * @param name Character name (case-sensitive)
     * @return Character index if found, std::nullopt otherwise
     */
    static std::optional<size_t> findCharacterByName(const std::string& name);

    /**
     * @brief Get character by name with fallback
     * @param name Character name
     * @param fallback_index Fallback index if name not found (default: 0)
     * @return Const reference to character
     */
    static const NodeCharacter& getCharacterByNameOrDefault(
        const std::string& name,
        size_t fallback_index = 0
    );

    /**
     * @brief Validate character parameters
     * @param character Character to validate
     * @return True if valid, false otherwise
     */
    static bool validateCharacter(const NodeCharacter& character);

    /**
     * @brief Get character name by index
     * @param index Character ID
     * @return Character name, or "Unknown" if invalid
     */
    static const char* getCharacterName(size_t index);

    /**
     * @brief Get character description by index
     * @param index Character ID
     * @return Character description, or "" if invalid
     */
    static const char* getCharacterDescription(size_t index);

    // Future expansion: user preset management
    // static bool saveUserCharacter(const NodeCharacter& character, const std::string& name);
    // static std::vector<NodeCharacter> getUserCharacters();
};

#endif // CHARACTER_MANAGER_H
```

```cpp
// CharacterManager.cpp

#include "CharacterManager.h"

const NodeCharacter& CharacterManager::getCharacter(size_t index) {
    if (index >= getCharacterCount()) {
        return CHARACTER_LIBRARY[0];  // Return default
    }
    return CHARACTER_LIBRARY[index];
}

size_t CharacterManager::getCharacterCount() {
    return NUM_BUILTIN_CHARACTERS;
}

std::optional<size_t> CharacterManager::findCharacterByName(const std::string& name) {
    for (size_t i = 0; i < getCharacterCount(); ++i) {
        if (name == CHARACTER_LIBRARY[i].name) {
            return i;
        }
    }
    return std::nullopt;
}

const NodeCharacter& CharacterManager::getCharacterByNameOrDefault(
    const std::string& name,
    size_t fallback_index
) {
    auto index = findCharacterByName(name);
    return getCharacter(index.value_or(fallback_index));
}

bool CharacterManager::validateCharacter(const NodeCharacter& character) {
    // Validate mode parameters
    for (int i = 0; i < 4; i++) {
        if (character.mode_freq_mult[i] < 0.1f || character.mode_freq_mult[i] > 20.0f) {
            return false;
        }
        if (character.mode_damping[i] < 0.01f || character.mode_damping[i] > 10.0f) {
            return false;
        }
        if (character.mode_weight[i] < 0.0f || character.mode_weight[i] > 1.0f) {
            return false;
        }
    }

    // Validate excitation parameters
    if (character.poke_strength < 0.0f || character.poke_strength > 1.0f) {
        return false;
    }
    if (character.poke_duration_ms < 1.0f || character.poke_duration_ms > 50.0f) {
        return false;
    }
    if (character.coupling_response_gain < 0.1f || character.coupling_response_gain > 2.0f) {
        return false;
    }

    return true;
}

const char* CharacterManager::getCharacterName(size_t index) {
    return getCharacter(index).name;
}

const char* CharacterManager::getCharacterDescription(size_t index) {
    return getCharacter(index).description;
}
```

### Phase 3: Remove Legacy 3-Value System

#### 3.1 Delete Legacy Code

**Files to modify:**

1. **modal_node.c** - Remove MODAL_PRESETS and related functions
   ```c
   // DELETE LINES 60-211 (mode_preset_t, modal_preset_t, MODAL_PRESETS[])
   // DELETE LINES 370-410 (modal_node_apply_preset, modal_node_get_preset_name, etc.)
   ```

2. **modal_node.h** - Remove legacy API declarations
   ```c
   // DELETE function declarations:
   // - void modal_node_apply_preset(...)
   // - const char* modal_node_get_preset_name(...)
   // - uint8_t modal_node_get_num_presets(void)
   // - const char* modal_node_get_preset_description(...)
   ```

3. **test_presets.c** - Delete or update to use CharacterManager
   ```bash
   # Option 1: Delete test program (no longer needed)
   rm ModalAttractors/ModalAttractorsExtension/DSP/test_presets.c

   # Option 2: Rewrite to test CharacterManager (optional)
   ```

4. **PHYSICAL_PRESETS.md** - Update documentation
   ```markdown
   # Delete or update to reference NodeCharacter system
   # Point users to CharacterManager API instead
   ```

#### 3.2 Update NodeManager to use CharacterManager

**File:** `ModalAttractors/ModalAttractorsExtension/DSP/NodeManager.cpp`

**Current:**
```cpp
void NodeManager::setNodeCharacter(uint8_t node_idx, uint8_t character_id) {
    if (node_idx >= NUM_NETWORK_NODES) return;

    const NodeCharacter* character = getCharacter(character_id);  // Uses old API
    if (!character) return;

    node_character_ids_[node_idx] = character_id;
    applyCharacterToNode(node_idx, character);
}
```

**Updated:**
```cpp
#include "CharacterManager.h"

void NodeManager::setNodeCharacter(uint8_t node_idx, uint8_t character_id) {
    if (node_idx >= NUM_NETWORK_NODES) return;

    const NodeCharacter& character = CharacterManager::getCharacter(character_id);

    node_character_ids_[node_idx] = character_id;
    applyCharacterToNode(node_idx, &character);
}
```

### Phase 4: Update Swift UI Layer

#### 4.1 Update CharacterTemplates.swift

**File:** `ModalAttractors/ModalAttractorsFramework/UI/Utilities/CharacterTemplates.swift`

**Action:** Update to match new C++ CHARACTER_LIBRARY (19 characters)

```swift
public struct CharacterTemplates {
    public static let all: [CharacterTemplate] = [
        // 0-14: Original characters (unchanged)
        vibrantBass, darkNode, brightBell, glassyShimmer, droneHub,
        metallicStrike, warmPad, percussiveHit, resonantBell, deepRumble,
        harmonicStack, detunedChorus, malletTone, windChime, gongWash,

        // 15-18: New physically-derived presets
        churchBell,        // 15
        circularPlate,     // 16
        wineGlass,         // 17
        tunedBar,          // 18
        drumMembrane,      // 19
        smallBell,         // 20

        // REMOVED: freeBar (duplicate of brightBell)
        // REMOVED: harmonicString (duplicate of harmonicStack)
    ]

    // Add new physical preset definitions:
    public static let churchBell = CharacterTemplate(
        name: "Church Bell",
        description: "Western church bell with hum, fundamental, tierce, and quint",
        mode0: (1.0, 1.0, 1.0),
        mode1: (1.19, 1.2, 0.75),
        mode2: (1.5, 1.4, 0.6),
        mode3: (2.0, 1.8, 0.45),
        waveShapes: (0, 0, 0, 0),  // All sine
        pokeStrength: 0.7,
        pokeDuration: 10.0,
        personality: 0  // Resonator
    )

    // ... (add remaining 5 physical presets)
}
```

**Result:** Swift templates now exactly match C++ CHARACTER_LIBRARY

#### 4.2 Update NodeCharactersView.swift

**File:** `ModalAttractors/ModalAttractorsFramework/UI/Views/NodeCharactersView.swift`

**Current:** Hardcoded picker range `0..<15`

**Updated:**
```swift
Picker("Character", selection: $selectedCharacterID) {
    ForEach(0..<CharacterTemplates.all.count, id: \.self) { index in
        Text(CharacterTemplates.all[index].name)
            .tag(index)
    }
}
```

**Result:** Picker automatically shows all 21 characters

#### 4.3 Verify CharacterPresetManager Compatibility

**File:** `ModalAttractors/ModalAttractorsFramework/UI/Utilities/CharacterPresetManager.swift`

**Status:** ✅ No changes needed - already uses dictionary-based storage

The preset manager stores parameters by key-value pairs, so adding new characters doesn't break compatibility.

### Phase 5: Update Parameter System

#### 5.1 Parameter Address Updates

**File:** `ModalAttractors/ModalAttractorsExtension/Parameters/Parameters.swift`

**Current:**
```swift
// Node character parameters (0-14)
param_Node0_Character = 20
param_Node1_Character = 21
// ...
```

**Updated:**
```swift
// Node character parameters (0-20) - expanded range
// No code changes needed - just update comments
param_Node0_Character = 20  // Range: 0-20 (was 0-14)
param_Node1_Character = 21  // Range: 0-20 (was 0-14)
// ...
```

**Action:** Update parameter max values in `createParameters()`:
```swift
let node0Char = AUParameterTree.createParameter(
    withIdentifier: "node0Character",
    name: "Node 0 Character",
    address: AUParameterAddress(param_Node0_Character.rawValue),
    min: 0,
    max: 20,  // Changed from 14
    unit: .indexed,
    unitName: nil,
    flags: [.flag_IsReadable, .flag_IsWritable],
    valueStrings: CharacterTemplates.all.map { $0.name },  // Auto-generate from array
    dependentParameters: nil
)
```

#### 5.2 Update SynthEngine Parameter Handling

**File:** `ModalAttractors/ModalAttractorsExtension/DSP/SynthEngine.cpp`

**Current:** No validation on character ID range

**Updated:** Add bounds checking (CharacterManager handles this)
```cpp
case kParam_Node0_Character:
case kParam_Node1_Character:
case kParam_Node2_Character:
case kParam_Node3_Character:
case kParam_Node4_Character: {
    uint8_t nodeIdx = paramId - kParam_Node0_Character;
    uint8_t characterID = static_cast<uint8_t>(value);

    // Bounds check (CharacterManager will clamp)
    if (characterID >= CharacterManager::getCharacterCount()) {
        characterID = 0;
    }

    nodeManager_->setNodeCharacter(nodeIdx, characterID);
    break;
}
```

---

## Implementation Plan (Detailed Steps)

### Step 1: Create CharacterManager Foundation
**Estimated effort:** 2 hours

1. Create new files:
   - `ModalAttractors/ModalAttractorsExtension/DSP/CharacterManager.h`
   - `ModalAttractors/ModalAttractorsExtension/DSP/CharacterManager.cpp`

2. Implement CharacterManager class (see Phase 2 above)

3. Add to build system:
   - Update `CMakeLists.txt` (if using CMake)
   - Add to Xcode project

4. Write unit tests (optional but recommended):
   - Test getCharacter() with valid/invalid indices
   - Test findCharacterByName()
   - Test validateCharacter()

### Step 2: Expand CHARACTER_LIBRARY
**Estimated effort:** 3 hours

1. Add 6 new physical presets to `NodeCharacter.cpp`:
   - Church Bell (freq: 1.0, 1.19, 1.5, 2.0)
   - Circular Plate (freq: 1.0, 2.081, 3.413, 3.891)
   - Wine Glass (freq: 1.0, 2.28, 3.65, 5.13)
   - Tuned Bar (freq: 1.0, 4.0, 10.0, 18.0)
   - Drum Membrane (freq: 1.0, 1.593, 2.136, 2.296)
   - Small Bell (freq: 1.0, 1.35, 1.7, 2.2)

2. For each preset, set reasonable defaults:
   - `mode_shape`: All WAVE_SHAPE_SINE
   - `personality`: PERSONALITY_RESONATOR
   - `poke_strength`: 0.6-0.8 (tune by ear)
   - `poke_duration_ms`: 8-15ms (tune by ear)
   - `coupling_response_gain`: 1.0

3. Update `NUM_BUILTIN_CHARACTERS` in `NodeCharacter.h`:
   ```cpp
   #define NUM_BUILTIN_CHARACTERS 21  // Was 15
   ```

4. Convert to array-driven structure:
   - Remove individual `CHARACTER_*` constants
   - Create single `CHARACTER_LIBRARY[]` array
   - Update `NUM_BUILTIN_CHARACTERS` to use `sizeof()`

### Step 3: Remove Legacy MODAL_PRESETS
**Estimated effort:** 1 hour

1. Delete from `modal_node.c`:
   - Lines 60-211: `mode_preset_t`, `modal_preset_t`, `MODAL_PRESETS[]`
   - Lines 370-410: `modal_node_apply_preset()`, etc.

2. Delete from `modal_node.h`:
   - Legacy function declarations

3. Delete obsolete files:
   ```bash
   rm ModalAttractors/ModalAttractorsExtension/DSP/test_presets.c
   rm ModalAttractors/ModalAttractorsExtension/DSP/PHYSICAL_PRESETS.md
   ```

4. Search for any remaining references:
   ```bash
   grep -r "MODAL_PRESETS" ModalAttractors/
   grep -r "modal_node_apply_preset" ModalAttractors/
   grep -r "mode_preset_t" ModalAttractors/
   ```

### Step 4: Update DSP Integration
**Estimated effort:** 2 hours

1. Update `NodeManager.cpp`:
   - Add `#include "CharacterManager.h"`
   - Replace `getCharacter()` calls with `CharacterManager::getCharacter()`

2. Update `SynthEngine.cpp`:
   - Add bounds checking for character IDs
   - Use `CharacterManager::getCharacterCount()` for validation

3. Verify compilation:
   ```bash
   # Build DSP extension
   xcodebuild -project ModalAttractors.xcodeproj -scheme ModalAttractorsExtension
   ```

### Step 5: Update Swift UI Layer
**Estimated effort:** 2 hours

1. Update `CharacterTemplates.swift`:
   - Add 6 new physical preset templates
   - Remove duplicates (freeBar, harmonicString)
   - Verify count matches C++ (21 characters)

2. Update `NodeCharactersView.swift`:
   - Change picker range from `0..<15` to `0..<CharacterTemplates.all.count`
   - Verify dropdown shows all 21 characters

3. Update `Parameters.swift`:
   - Change character parameter max from 14 to 20
   - Update valueStrings to auto-generate from CharacterTemplates.all

4. Test CharacterPresetManager:
   - Load existing user presets
   - Verify no crashes with new character count
   - Save new preset with physical characters

### Step 6: Testing & Validation
**Estimated effort:** 3 hours

1. **Build verification:**
   ```bash
   # Clean build
   xcodebuild clean -project ModalAttractors.xcodeproj
   xcodebuild -project ModalAttractors.xcodeproj -scheme ModalAttractorsExtension
   xcodebuild -project ModalAttractors.xcodeproj -scheme ModalAttractorsFramework
   ```

2. **Unit tests (if available):**
   - Test CharacterManager API
   - Test character parameter ranges
   - Test NodeManager character application

3. **Integration tests:**
   - Load plugin in Logic Pro / Standalone
   - Test all 21 characters in Node Character picker
   - Verify each character produces sound
   - Test preset save/load with new characters
   - Test parameter automation

4. **Audio validation:**
   - Play notes with each physical preset
   - Verify Church Bell sounds bell-like
   - Verify Circular Plate sounds metallic
   - Verify no clicks/pops when switching characters

5. **UI validation:**
   - All 21 characters appear in dropdown
   - Character names display correctly
   - Template application works
   - Preset manager doesn't crash

### Step 7: Documentation Updates
**Estimated effort:** 1 hour

1. Update README.md:
   - Document CharacterManager API
   - List all 21 built-in characters
   - Update preset count (21 vs old 15+8)

2. Update inline code comments:
   - Update `NodeCharacter.h` header comments
   - Document character ID ranges (0-20)

3. Create migration guide (if needed):
   - Note that old character IDs 0-14 remain unchanged
   - New characters are IDs 15-20
   - No breaking changes for existing sessions

---

## Compatibility & Migration

### Backward Compatibility

**Session files:**
- ✅ **SAFE:** Old character IDs 0-14 remain unchanged
- ✅ **SAFE:** New characters use IDs 15-20 (no conflicts)
- ✅ **SAFE:** Parameter addresses unchanged
- ✅ **SAFE:** fullState dictionary format unchanged

**User presets:**
- ✅ **SAFE:** CharacterPresetManager uses dictionary storage
- ✅ **SAFE:** Adding new characters doesn't break existing presets
- ✅ **SAFE:** Parameter keys remain the same

**Breaking changes:**
- ❌ **NONE** - This is a pure addition, no breaking changes

### Migration Strategy

**Development stage assumptions:**
- No released versions exist
- No user sessions to preserve
- Free to renumber/restructure

**If you want to preserve existing dev sessions:**
1. Keep character IDs 0-14 unchanged
2. Append new characters as IDs 15-20
3. Parameter addresses unchanged
4. fullState loading will work seamlessly

---

## Validation Checklist

### Pre-Implementation
- [x] Understand current architecture
- [x] Identify all preset/character references
- [x] Map duplicate presets
- [x] Document parameter flow

### During Implementation
- [ ] CharacterManager compiles without errors
- [ ] CHARACTER_LIBRARY expanded to 21 characters
- [ ] Legacy MODAL_PRESETS code removed
- [ ] No remaining references to `mode_preset_t`
- [ ] NodeManager uses CharacterManager API
- [ ] Swift CharacterTemplates matches C++ count
- [ ] Parameter max values updated to 20

### Post-Implementation
- [ ] Clean build succeeds
- [ ] Plugin loads in Logic Pro / Standalone
- [ ] All 21 characters selectable in UI
- [ ] Each character produces distinct sound
- [ ] Preset save/load works
- [ ] Parameter automation works
- [ ] No crashes or memory leaks
- [ ] Documentation updated

---

## Search & Destroy Checklist

Use these grep patterns to find remaining legacy code:

```bash
# Legacy preset structure
grep -r "mode_preset_t" ModalAttractors/ --include="*.c" --include="*.h"
grep -r "modal_preset_t" ModalAttractors/ --include="*.c" --include="*.h"
grep -r "MODAL_PRESETS" ModalAttractors/ --include="*.c" --include="*.h"

# Legacy API functions
grep -r "modal_node_apply_preset" ModalAttractors/ --include="*.c" --include="*.h" --include="*.cpp"
grep -r "modal_node_get_preset_name" ModalAttractors/ --include="*.c" --include="*.h" --include="*.cpp"
grep -r "modal_node_get_num_presets" ModalAttractors/ --include="*.c" --include="*.h" --include="*.cpp"

# Legacy field names (should use NodeCharacter fields instead)
grep -r "freq_ratio" ModalAttractors/ --include="*.c" --include="*.h"
grep -r "damping_ratio" ModalAttractors/ --include="*.c" --include="*.h"

# Old character constant references (after array conversion)
grep -r "CHARACTER_VIBRANT_BASS" ModalAttractors/ --include="*.cpp" --include="*.h"
grep -r "&CHARACTER_" ModalAttractors/ --include="*.cpp" --include="*.h"

# Verify new API usage
grep -r "CharacterManager::" ModalAttractors/ --include="*.cpp"
grep -r "getCharacter(" ModalAttractors/ --include="*.cpp"
```

**Expected results after cleanup:**
- `mode_preset_t`: 0 matches
- `MODAL_PRESETS`: 0 matches
- `modal_node_apply_preset`: 0 matches
- `CharacterManager::`: 5+ matches (NodeManager.cpp, SynthEngine.cpp)

---

## Risk Assessment

### Low Risk
- ✅ Adding new characters (IDs 15-20) doesn't affect existing 0-14
- ✅ CharacterManager is a new class, doesn't modify existing code
- ✅ Swift UI changes are additive (more items in picker)

### Medium Risk
- ⚠️ Removing MODAL_PRESETS: Ensure no hidden dependencies
- ⚠️ Converting to array structure: Verify CHARACTER_LIBRARY indexing
- ⚠️ Parameter max value changes: Test edge cases (automation clips)

### Mitigation
- Create feature branch for testing
- Run full regression test suite
- Test in Logic Pro with existing projects
- Keep git history clean for easy rollback

---

## Future Enhancements

After consolidation is complete, consider:

1. **User preset expansion:**
   - Allow users to save custom characters to disk
   - CharacterManager::saveUserCharacter()
   - CharacterManager::getUserCharacters()

2. **Preset categories:**
   - Tag characters: "Physical", "Synthetic", "Harmonic", "Inharmonic"
   - Filter UI by category

3. **Preset morphing:**
   - Interpolate between two characters
   - CharacterManager::morphCharacters(charA, charB, amount)

4. **Preset versioning:**
   - Add version field to NodeCharacter
   - Support migration for future character format changes

5. **Preset validation:**
   - Audio-based validation (measure THD, decay time, etc.)
   - Ensure characters sound as intended

---

## Summary

This proposal consolidates two parallel preset systems into one unified, maintainable architecture:

**Before:**
- 2 separate systems (MODAL_PRESETS + CHARACTER_LIBRARY)
- 3-value legacy structure (limited expressiveness)
- 23 total templates in Swift (manual sync)
- Scattered constants in C++
- No unified API

**After:**
- 1 unified system (NodeCharacter array)
- Rich 11-field structure (full expressiveness)
- 21 total characters (duplicates removed)
- Array-driven in C++
- CharacterManager class API

**Benefits:**
- ✅ Single source of truth
- ✅ Easier to add new characters
- ✅ Type-safe API
- ✅ No duplication between C++ and Swift
- ✅ Better validation
- ✅ Future-proof for user presets

**Effort estimate:** ~14 hours total

**Timeline suggestion:**
- Day 1: Steps 1-3 (CharacterManager + expand library + remove legacy)
- Day 2: Steps 4-5 (DSP integration + Swift UI)
- Day 3: Steps 6-7 (testing + documentation)
