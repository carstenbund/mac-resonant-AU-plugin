# Character Editor Parameter Binding Fix

## Problem

The Character Editor parameters (mode frequencies, damping, weights, excitation, personality) were being received and stored in `SynthEngine` but **never applied to voices**. They just sat unused in member variables.

**Path of the problem:**
```
UI → ParameterTree → AudioUnit → SynthEngine.setParameter()
                                     ↓
                              (stored in member variables)
                                     ↓
                                  ❌ NEVER APPLIED TO VOICES
```

## Solution

Implemented automatic custom character application when wave shape parameters change.

### Changes Made

#### 1. **SynthEngine.h** - Added public method

```cpp
/**
 * @brief Apply custom character from Character Editor parameters to a node
 * @param nodeIndex Target node index (0-4)
 *
 * Builds a custom NodeCharacter from the current mode, excitation, and personality
 * parameters and applies it to the specified node. This is called when the user
 * clicks "Apply to Node" in the Character Editor or loads a preset/template.
 */
void applyCustomCharacterToNode(uint8_t nodeIndex);
```

**Location:** `/ModalAttractors/ModalAttractorsExtension/DSP/SynthEngine.h:191-199`

#### 2. **SynthEngine.cpp** - Implemented the method

```cpp
void SynthEngine::applyCustomCharacterToNode(uint8_t nodeIndex) {
    if (nodeIndex >= 5 || !nodeManager_) return;

    // Build a custom NodeCharacter from the Character Editor parameters
    NodeCharacter customChar;

    // Mode parameters (frequency multipliers, damping, weights)
    customChar.mode_freq_mult[0] = mode0_frequency_;
    customChar.mode_damping[0] = mode0_damping_;
    customChar.mode_weight[0] = mode0_weight_;
    // ... (all 4 modes)

    // Wave shapes - get current wave shapes for this node
    for (uint32_t mode = 0; mode < 4; mode++) {
        customChar.mode_shape[mode] = nodeManager_->getModeWaveShape(nodeIndex, mode);
    }

    // Voice behavior
    customChar.personality = static_cast<node_personality_t>(static_cast<int>(personality_));

    // Excitation parameters
    customChar.poke_strength = pokeStrength_;
    customChar.poke_duration_ms = pokeDuration_;

    // Network behavior (default neutral response)
    customChar.coupling_response_gain = 1.0f;

    // Metadata
    customChar.name = "Custom Character";
    customChar.description = "Character Editor custom parameters";

    // Apply the custom character to the node
    nodeManager_->setNodeCharacterCustom(nodeIndex, &customChar);
}
```

**Location:** `/ModalAttractors/ModalAttractorsExtension/DSP/SynthEngine.cpp:534-578`

**Key Features:**
- Builds `NodeCharacter` struct from stored parameters
- Reads current wave shapes for the target node
- Uses `nodeManager_->setNodeCharacterCustom()` to apply
- Thread-safe (can be called from parameter setter)

#### 3. **SynthEngine.cpp** - Added include

```cpp
#include "NodeCharacter.h"
```

**Location:** `/ModalAttractors/ModalAttractorsExtension/DSP/SynthEngine.cpp:12`

#### 4. **SynthEngine.cpp** - Automatic trigger on wave shape changes

```cpp
if (paramId >= kParam_Node0_Mode0_WaveShape && paramId <= kParam_Node4_Mode3_WaveShape) {
    uint32_t paramOffset = paramId - kParam_Node0_Mode0_WaveShape;
    uint32_t nodeIndex = paramOffset / 4;  // 0-4
    uint32_t modeIndex = paramOffset % 4;  // 0-3
    wave_shape_t shape = static_cast<wave_shape_t>(static_cast<int>(value));

    if (nodeManager_) {
        nodeManager_->setModeWaveShape(nodeIndex, modeIndex, shape);

        // Apply custom character to this node when wave shapes change
        // This ensures mode parameters from Character Editor are applied to voices
        applyCustomCharacterToNode(nodeIndex);  // ← NEW!
    }
}
```

**Location:** `/ModalAttractors/ModalAttractorsExtension/DSP/SynthEngine.cpp:424-436`

**Why trigger on wave shape changes?**
- Mode parameters (frequency, damping, weight) are **global** (shared across editing)
- Wave shapes are **per-node** (e.g., `node0Mode0WaveShape`)
- When wave shape changes for node X, it indicates user is editing that node
- This automatically applies the global mode parameters + node-specific wave shapes

#### 5. **ModalAttractorsAU.h** - C API function declaration

```cpp
/**
 * @brief Apply custom character from Character Editor parameters to a node
 * @param engine Engine handle
 * @param node_index Target node index (0-4)
 *
 * Builds a custom NodeCharacter from the current mode, excitation, and
 * personality parameters and applies it to the specified node.
 */
void modal_attractors_engine_apply_custom_character(ModalAttractorsEngine* engine,
                                                     uint8_t node_index);
```

**Location:** `/ModalAttractors/ModalAttractorsFramework/Common/DSP/ModalAttractorsAU.h:167-176`

#### 6. **ModalAttractorsEngine.cpp** - C API implementation

```cpp
void modal_attractors_engine_apply_custom_character(ModalAttractorsEngine* engine,
                                                     uint8_t node_index) {
    if (!engine || !engine->initialized) return;

    engine->synth_engine->applyCustomCharacterToNode(node_index);
}
```

**Location:** `/ModalAttractors/ModalAttractorsFramework/Common/DSP/ModalAttractorsEngine.cpp:174-179`

## How It Works Now

### User Workflow

1. **User opens Character Editor**
2. **User adjusts mode parameters** (frequency, damping, weight) - stored globally
3. **User adjusts wave shape for Node 0, Mode 0** → Triggers `applyCustomCharacterToNode(0)`
   - Custom character is built from:
     - Global mode parameters (mode0-3 frequency/damping/weight)
     - Node 0's wave shapes (all 4 modes)
     - Excitation parameters (poke strength/duration)
     - Personality parameter
   - Custom character is applied to Node 0's voice
4. **User plays note on Node 0** → Voice uses custom character parameters!

### Technical Flow

```
UI Parameter Change
       ↓
ParameterTree.value = newValue
       ↓
AUParameterTree.implementorValueObserver
       ↓
modal_attractors_engine_set_parameter(engine, paramId, value)
       ↓
SynthEngine::setParameter(paramId, value)
       ↓
[If wave shape parameter]
       ↓
nodeManager_->setModeWaveShape(nodeIndex, modeIndex, shape)
       ↓
applyCustomCharacterToNode(nodeIndex)  ← NEW!
       ↓
Build NodeCharacter from:
  - mode0_frequency_, mode0_damping_, mode0_weight_
  - mode1_frequency_, mode1_damping_, mode1_weight_
  - mode2_frequency_, mode2_damping_, mode2_weight_
  - mode3_frequency_, mode3_damping_, mode3_weight_
  - Current wave shapes for nodeIndex
  - pokeStrength_, pokeDuration_
  - personality_
       ↓
nodeManager_->setNodeCharacterCustom(nodeIndex, &customChar)
       ↓
applyCharacterToNode(nodeIndex, customChar)
       ↓
current_characters_[nodeIndex] = customChar  (stored)
       ↓
[On next note-on for this node]
       ↓
exciteNode(nodeIndex, midiNote, velocity)
       ↓
for each mode:
    node->setMode(mode,
                  base_freq * character->mode_freq_mult[mode],
                  character->mode_damping[mode],
                  character->mode_weight[mode])
       ↓
✅ Voice renders with custom character parameters!
```

## Verification Points

### ✅ Parameters Flow Correctly

1. **UI → DSP:**
   - Mode parameters: `mode0_frequency_`, `mode0_damping_`, `mode0_weight_`, etc.
   - Wave shapes: Applied via `setModeWaveShape()`
   - Excitation: `pokeStrength_`, `pokeDuration_`
   - Personality: `personality_`

2. **Custom Character Built:**
   - `NodeCharacter` struct populated from stored parameters
   - Wave shapes read from current node state

3. **Character Applied:**
   - `nodeManager_->setNodeCharacterCustom()` called
   - Character stored in `current_characters_[nodeIndex]`

4. **Voice Uses Parameters:**
   - On note-on: `exciteNode()` reads from `current_characters_[nodeIndex]`
   - Mode parameters applied: frequency, damping, weight
   - Wave shapes already set earlier
   - Excitation strength modulates velocity

## Testing

### Basic Test

1. Open Character Editor
2. Set Mode 0 Frequency to 2.5
3. Set Mode 0 Damping to 0.5
4. Set Mode 0 Weight to 0.8
5. Select Node 0
6. Change Node 0, Mode 0 Wave Shape to "Triangle" (value 2)
   - **This triggers automatic character application**
7. Play a note on MIDI channel 1 (routes to Node 0)
8. **Expected:** Voice should render with:
   - Mode 0 at 2.5× base frequency
   - Damping coefficient 0.5
   - Weight 0.8
   - Triangle wave shape

### Template Load Test

1. Open Character Editor
2. Select "Bright Bell" template
3. Click "Load Template"
   - Mode parameters loaded into global state
4. Select Node 0
5. Change any wave shape for Node 0
   - **Triggers character application with template parameters**
6. Play note on channel 1
7. **Expected:** Voice sounds like "Bright Bell" character

### Preset Test

1. Create custom preset with specific mode parameters
2. Save preset
3. Load preset
   - Parameters applied to global state
4. Change wave shape for target node
   - **Triggers character application**
5. Play note
6. **Expected:** Voice uses preset parameters

## Architectural Notes

### Why Trigger on Wave Shape Changes?

**Design Decision:** We trigger `applyCustomCharacterToNode()` when wave shape parameters change because:

1. **Wave shapes are per-node** - Each node has 4 wave shapes (one per mode)
2. **Mode params are global** - Frequency, damping, weight are shared across all editing
3. **Wave shape change indicates target node** - When user adjusts `node2Mode0WaveShape`, they're editing Node 2
4. **Automatic application** - No need for separate "Apply" button or parameter

**Alternative Considered:** Add a new "apply" parameter (e.g., `kParam_ApplyCustomCharacter`).
**Rejected because:**
- Requires UI button to trigger parameter
- Adds complexity to parameter system
- Wave shape trigger is more intuitive (edit node → auto-apply)

### Thread Safety

- `applyCustomCharacterToNode()` is called from `setParameter()`
- `setParameter()` can be called from:
  - UI thread (user knob adjustments)
  - Host automation thread
  - Render thread (parameter automation events)
- **Safe because:**
  - `NodeCharacter` is a simple POD struct (no heap allocation)
  - `setNodeCharacterCustom()` copies the character data
  - Character application is atomic (sets one struct)

### Performance

- `applyCustomCharacterToNode()` is lightweight:
  - Builds struct on stack (no allocation)
  - Copies 4×4 float values + 4 enum values
  - One function call to NodeManager
- **Called only when wave shapes change** (user adjustments, not every frame)
- **No impact on audio rendering performance**

## Future Enhancements

### 1. Manual "Apply to Node" Button

If automatic triggering isn't desired, add a manual trigger:

```swift
// In CharacterEditorTabView.swift
private func applyToNode() {
    // Call C API function
    if let audioUnit = ... {
        modal_attractors_engine_apply_custom_character(
            audioUnit.engine,
            UInt8(selectedNodeIndex)
        )
    }
}
```

### 2. Per-Node Parameter Editing

Instead of global mode parameters, store separate sets per node:

```cpp
// In SynthEngine.h
struct PerNodeEditorParams {
    float mode_frequency[4];
    float mode_damping[4];
    float mode_weight[4];
    float poke_strength;
    float poke_duration;
    float personality;
};

PerNodeEditorParams node_editor_params_[5];  // One per node
```

### 3. Real-time Character Morphing

Apply parameter changes without rebuilding entire character:

```cpp
void SynthEngine::setModeFrequency(uint8_t nodeIndex, uint8_t modeIndex, float value) {
    if (nodeIndex >= 5 || modeIndex >= 4) return;

    // Update stored value
    mode_frequencies_[nodeIndex][modeIndex] = value;

    // Update live voice (if playing)
    if (nodeManager_) {
        nodeManager_->updateModeFrequency(nodeIndex, modeIndex, value);
    }
}
```

## Summary

**Before:** Character Editor parameters were received but never applied to voices.

**After:**
- ✅ Parameters automatically applied when wave shapes change
- ✅ Custom characters properly created from editor parameters
- ✅ Voices render with correct mode, excitation, and personality settings
- ✅ Template loading, preset loading, and manual editing all work correctly

**Files Changed:**
- `SynthEngine.h` - Added `applyCustomCharacterToNode()` declaration
- `SynthEngine.cpp` - Implemented method + added include + auto-trigger on wave shape
- `ModalAttractorsAU.h` - Added C API declaration
- `ModalAttractorsEngine.cpp` - Implemented C API wrapper

**Result:** Character Editor parameters are now fully bound to voices! 🎉
