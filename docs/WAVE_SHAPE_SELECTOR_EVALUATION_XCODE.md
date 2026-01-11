# Wave Shape Selector Evaluation - Xcode Project
## ModalAttractors AUv3 Plugin

**Date:** 2026-01-10
**Status:** Evaluation for Implementation
**Based on:** Actual Xcode project in `ModalAttractors/ModalAttractorsExtension/`

---

## Executive Summary

**YES** - Adding wave shape selection (sine, sawtooth, triangle, square, pulse) to the modal resonator is **highly feasible** and would significantly expand the sonic palette without compromising the unique modal synthesis architecture.

**Key Finding**: The current phase accumulator system (per-mode, per-node) is perfectly suited for wave shape variation. Each of the 20 oscillators (5 nodes × 4 modes) currently generates sine waves. Allowing wave shape selection would enable:
- **Spectral richness** from harmonic-rich waveforms (sawtooth, square)
- **Timbral morphing** as modal amplitudes evolve
- **Network interactions** with more complex harmonic content
- **Compatibility** with existing character system (characters could specify wave shapes)

---

## Current Architecture Analysis

### System Overview

**Architecture:** 5-Node Network with Modal Synthesis

```
5 Nodes (each with its own Character)
  └── 4 Modes per Node
        └── Phase Accumulator per Mode
              └── Sinusoid Generation (SINE ONLY - Line 163)
```

**Total Oscillators:** 20 independent sine oscillators (5 nodes × 4 modes)

### How Audio is Currently Generated

**Location:** `ModalAttractors/ModalAttractorsExtension/DSP/audio_synth.c:91-181`

**Core Rendering Loop:**

```c
// Line 118-180: Main render loop
for (uint32_t sample_idx = 0; sample_idx < num_frames; sample_idx++) {
    float sample_sum = 0.0f;

    for (int k = 0; k < MAX_MODES; k++) {  // 4 modes per node
        // Line 128-141: Extract mode amplitude from modal dynamics
        float amplitude_raw = cabsf(node->modes[k].a);  // |a_k|
        amplitude_raw *= node->modes[k].params.weight;

        // Smooth amplitude (avoid clicks)
        synth->amplitude_smooth[k] +=
            SMOOTH_ALPHA * (amplitude_raw - synth->amplitude_smooth[k]);

        // Final amplitude with gains
        float amplitude = synth->amplitude_smooth[k] *
                        synth->params.mode_gains[k] *
                        synth->params.master_gain *
                        MAX_AMPLITUDE_SCALE;

        // Line 148-157: Phase calculation
        float omega = node->modes[k].params.omega;  // rad/s
        float freq_hz = omega / (2.0f * M_PI);
        float phase_inc = 2.0f * M_PI * freq_hz / sample_rate;

        uint32_t phase_acc = synth->params.phase_accumulator[k];
        float phase = (phase_acc / 4294967296.0f) * 2.0f * M_PI;

        // *** LINE 163: SINGLE POINT OF WAVE GENERATION ***
        float sample_f = amplitude * sinf(phase);

        sample_sum += sample_f;

        // Line 168-170: Advance phase
        phase_acc += (uint32_t)(phase_inc * 4294967296.0f / (2.0f * M_PI));
        synth->params.phase_accumulator[k] = phase_acc;
    }

    // Clamp and write stereo output
    outL[sample_idx] = clamp(sample_sum, -1.0f, 1.0f);
    outR[sample_idx] = clamp(sample_sum, -1.0f, 1.0f);
}
```

**Critical Observation:**
- **Line 163** is the ONLY place where waveform generation happens
- Changing this single line enables arbitrary waveforms
- Phase accumulator already provides perfect [0, 2π) normalized phase
- Architecture is already optimal for wave shape selection

---

## Current Parameter System

**Parameter Definition:** `ModalAttractors/ModalAttractorsExtension/Parameters/Parameters.swift`

**Current Parameters (27 total):**

1. **Global (4 params)**
   - Master Gain
   - Coupling Strength
   - Topology (Ring, Small World, etc.)
   - Node Count (fixed at 5)

2. **Node Characters (5 params)**
   - Node 0-4 Character (0-14 indexed)
   - Each selects from 15 built-in character presets

3. **Mode Parameters (12 params)** - For Character Editor only
   - Mode 0-3: Frequency Multiplier (0.5-8.0)
   - Mode 0-3: Damping (0.1-5.0)
   - Mode 0-3: Weight (0.0-1.0)

4. **Excitation (2 params)**
   - Poke Strength
   - Poke Duration

5. **Routing (2 params)**
   - Note Routing (MIDI Channel vs All Nodes)
   - Multi-Excitation (Re-Trigger vs Accumulate)

6. **Voice (2 params)**
   - Polyphony (read-only, always 5)
   - Personality (Resonator vs Self-Oscillator)

**Parameter Addresses:** `ModalAttractorsExtensionParameterAddresses.h`
- Enum-based addressing (kParam_MasterGain = 0, etc.)
- Next available address: **27** (kParam_MultiExcite is last at 26)

---

## Character System Context

**Location:** `ModalAttractors/ModalAttractorsExtension/DSP/NodeCharacter.h/cpp`

**What is a Character?**

A character is a preset bundle defining:
```cpp
struct NodeCharacter {
    float mode_freq_mult[4];    // Frequency ratios (e.g., 1.0, 2.0, 3.0, 5.0)
    float mode_damping[4];      // Decay rates
    float mode_weight[4];       // Amplitude mix
    node_personality_t personality;
    float poke_strength;
    float poke_duration_ms;
    float coupling_response_gain;
};
```

**15 Built-in Characters:**
- Vibrant Bass, Dark Node, Bright Bell, Glassy Shimmer, Drone Hub
- Metallic Strike, Warm Pad, Percussive Hit, Resonant Bell, Deep Rumble
- Harmonic Stack, Detuned Chorus, Mallet Tone, Wind Chime, Gong Wash

**Current Workflow:**
1. User selects character for each of 5 nodes
2. Character defines frequency ratios and damping
3. Ratios create "timbre" through additive sine components
4. **All oscillators are still sine waves**

**Opportunity:** Wave shape selection would expand character expression beyond just frequency/damping ratios.

---

## Wave Shape Selector Design

### Option 1: Global Wave Shape (Simplest)

**Add one parameter that affects all 20 oscillators globally.**

**Pros:**
- Simplest implementation (~100 LOC)
- Easy UI (single selector)
- Maintains consistent timbre across network
- Users can still vary timbre through characters

**Cons:**
- Less flexibility than per-mode or per-node
- Cannot mix sine bass with sawtooth highs

**Implementation:**
```c
// audio_synth_params_t (audio_synth.h:41)
typedef struct {
    float sample_rate;
    uint32_t phase_accumulator[MAX_MODES];
    float mode_gains[MAX_MODES];
    float master_gain;
    bool muted;
    wave_shape_t wave_shape;     // NEW: Global wave shape
    float pulse_width;           // NEW: For pulse waves (0.01-0.99)
} audio_synth_params_t;
```

### Option 2: Per-Node Wave Shape (Recommended)

**Add 5 parameters (one per node) for wave shape selection.**

**Pros:**
- Excellent balance of flexibility and simplicity
- Nodes can have distinct timbral roles
- Natural extension of character system
- 5 controls is manageable in UI

**Cons:**
- More parameters to manage (5 new params)
- Slightly more complex implementation

**Implementation:**
```c
// modal_node.h: Add to modal_node_t
typedef struct {
    mode_params_t params;
    modal_state_t modes[MAX_MODES];
    wave_shape_t wave_shape;     // NEW: Wave shape for this node
    float pulse_width;           // NEW: Pulse width if using pulse wave
    // ... existing fields ...
} modal_node_t;
```

**Parameter Addition:**
```swift
// Parameters.swift
ParameterGroupSpec(identifier: "waveShapes", name: "Wave Shapes") {
    ParameterSpec(
        address: ModalAttractorsExtensionParameterAddress.kParam_Node0_WaveShape,
        identifier: "node0WaveShape",
        name: "Node 0 Wave",
        units: .indexed,
        valueRange: 0...5,
        defaultValue: 0,
        valueStrings: ["Sine", "Sawtooth", "Triangle", "Square", "Pulse 25%", "Pulse 10%"]
    )
    // Repeat for Node 1-4...
}
```

### Option 3: Per-Mode Wave Shape (Maximum Flexibility)

**Add 4 parameters per node (20 total) for per-mode wave selection.**

**Pros:**
- Ultimate flexibility
- Each mode can have different wave shape
- Enables complex spectral morphing

**Cons:**
- 20 new parameters (parameter explosion)
- Complex UI (4 selectors × 5 nodes = 20 controls)
- Overwhelming for most users
- Maintenance burden

**Recommendation:** **Not recommended** unless users specifically request it after Option 2.

### Option 4: Extend Character System (Most Elegant)

**Add wave shape to NodeCharacter definition, let characters specify wave shapes.**

**Pros:**
- No new user-facing parameters needed
- Characters naturally include wave shape
- Future characters can use different wave shapes
- Maintains simplicity of character-based workflow
- Expandable: add per-node wave override later

**Cons:**
- Less direct user control (must select character)
- Requires updating all 15 character definitions
- Users can't easily change wave without changing character

**Implementation:**
```cpp
// NodeCharacter.h
struct NodeCharacter {
    float mode_freq_mult[4];
    float mode_damping[4];
    float mode_weight[4];
    wave_shape_t mode_wave_shape[4];  // NEW: Wave shape per mode
    float mode_pulse_width[4];        // NEW: Pulse width per mode
    // ... existing fields ...
};
```

**Example Updated Characters:**
```cpp
// Character 0: Vibrant Bass - Now with sine bass + sawtooth harmonics
CHARACTER_VIBRANT_BASS = {
    .mode_freq_mult = {1.0f, 2.0f, 3.0f, 5.0f},
    .mode_damping = {0.5f, 0.6f, 0.8f, 1.2f},
    .mode_weight = {1.0f, 0.8f, 0.6f, 0.4f},
    .mode_wave_shape = {WAVE_SINE, WAVE_SINE, WAVE_SAW, WAVE_SAW},  // NEW
    .mode_pulse_width = {0.5f, 0.5f, 0.5f, 0.5f},  // Unused for sine/saw
    // ...
};
```

---

## Recommended Implementation Strategy

**Phase 1 (Recommended for Initial Implementation):**
Combine **Option 2 (Per-Node)** with **Option 4 (Character Integration)**

1. Add `wave_shape` to `modal_node_t` (per-node storage)
2. Add 5 parameter addresses (kParam_Node0_WaveShape through kParam_Node4_WaveShape)
3. Add wave shape selectors to Swift parameter tree
4. Implement oscillator functions in `audio_synth.c`
5. Update render loop to use switch statement
6. **Optionally:** Add wave shape to `NodeCharacter` so characters can specify default shapes

This provides:
- ✅ Per-node wave control (users can override)
- ✅ Character system can specify wave shapes
- ✅ Manageable parameter count (5 new params)
- ✅ Expandable to per-mode later if needed

---

## Implementation Details

### 1. Wave Shape Enum

**Location:** `ModalAttractors/ModalAttractorsExtension/DSP/audio_synth.h` (after line 32)

```c
/**
 * @brief Oscillator wave shapes
 */
typedef enum {
    WAVE_SHAPE_SINE = 0,      ///< Pure sine wave (default)
    WAVE_SHAPE_SAWTOOTH,      ///< Sawtooth (all harmonics, 1/n amplitude)
    WAVE_SHAPE_TRIANGLE,      ///< Triangle (odd harmonics, 1/n² amplitude)
    WAVE_SHAPE_SQUARE,        ///< Square wave (odd harmonics, 1/n amplitude)
    WAVE_SHAPE_PULSE_25,      ///< Pulse wave, 25% duty cycle
    WAVE_SHAPE_PULSE_10,      ///< Pulse wave, 10% duty cycle (thin)
    WAVE_SHAPE_COUNT          ///< Number of wave shapes
} wave_shape_t;
```

### 2. Oscillator Functions

**Location:** `ModalAttractors/ModalAttractorsExtension/DSP/audio_synth.c` (after line 57)

```c
/**
 * @brief Sawtooth oscillator (naive, will alias)
 * @param phase Phase in radians [0, 2π)
 * @return Sample value [-1, 1]
 */
static inline float osc_sawtooth(float phase) {
    // Descending sawtooth: 1 at phase=0, -1 at phase=2π
    return 1.0f - (phase / M_PI);
}

/**
 * @brief Triangle oscillator
 * @param phase Phase in radians [0, 2π)
 * @return Sample value [-1, 1]
 */
static inline float osc_triangle(float phase) {
    if (phase < M_PI) {
        // Rising edge: -1 to +1
        return -1.0f + (2.0f * phase / M_PI);
    } else {
        // Falling edge: +1 to -1
        return 3.0f - (2.0f * phase / M_PI);
    }
}

/**
 * @brief Square/pulse oscillator
 * @param phase Phase in radians [0, 2π)
 * @param pulse_width Pulse width [0, 1], where 0.5 = square
 * @return Sample value [-1, 1]
 */
static inline float osc_pulse(float phase, float pulse_width) {
    float threshold = pulse_width * 2.0f * M_PI;
    return (phase < threshold) ? 1.0f : -1.0f;
}

/**
 * @brief Sine oscillator (existing, kept for reference)
 * @param phase Phase in radians [0, 2π)
 * @return Sample value [-1, 1]
 */
static inline float osc_sine(float phase) {
    return sinf(phase);
}
```

### 3. Storage for Per-Node Wave Shape

**Location:** `ModalAttractors/ModalAttractorsExtension/DSP/modal_node.h` (add to modal_node_t)

```c
typedef struct {
    mode_params_t modes[MAX_MODES];
    modal_state_t state[MAX_MODES];

    // Excitation state
    float excitation_time;
    float excitation_duration;
    float excitation_strength;
    bool excitation_active;

    // NEW: Wave shape control
    wave_shape_t wave_shape;   ///< Oscillator wave shape for this node
    float pulse_width;         ///< Pulse width for pulse waves [0.01, 0.99]

    // Node dynamics
    node_personality_t personality;
    float coupling_input[MAX_MODES];
    bool initialized;
} modal_node_t;
```

**Initialize defaults in `modal_node_init()`:**
```c
void modal_node_init(modal_node_t* node, float sample_rate) {
    // ... existing initialization ...

    // Initialize wave shape to sine (backward compatible)
    node->wave_shape = WAVE_SHAPE_SINE;
    node->pulse_width = 0.5f;  // 50% duty cycle
}
```

### 4. Modify Render Loop

**Location:** `ModalAttractors/ModalAttractorsExtension/DSP/audio_synth.c:163`

**Replace:**
```c
// OLD (Line 163):
float sample_f = amplitude * sinf(phase);
```

**With:**
```c
// NEW: Generate sample with selected wave shape
float oscillator_out;
switch (node->wave_shape) {
    case WAVE_SHAPE_SINE:
        oscillator_out = sinf(phase);
        break;
    case WAVE_SHAPE_SAWTOOTH:
        oscillator_out = osc_sawtooth(phase);
        break;
    case WAVE_SHAPE_TRIANGLE:
        oscillator_out = osc_triangle(phase);
        break;
    case WAVE_SHAPE_SQUARE:
        oscillator_out = osc_pulse(phase, 0.5f);
        break;
    case WAVE_SHAPE_PULSE_25:
        oscillator_out = osc_pulse(phase, 0.25f);
        break;
    case WAVE_SHAPE_PULSE_10:
        oscillator_out = osc_pulse(phase, 0.1f);
        break;
    default:
        oscillator_out = sinf(phase);  // Fallback to sine
}

float sample_f = amplitude * oscillator_out;
```

### 5. Add Parameter Addresses

**Location:** `ModalAttractors/ModalAttractorsExtension/Parameters/ModalAttractorsExtensionParameterAddresses.h`

**Add after line 58 (after kParam_MultiExcite = 26):**

```objc
    // Wave Shape Control (NEW)
    kParam_Node0_WaveShape = 27,  // Wave shape for node 0 (0-5)
    kParam_Node1_WaveShape = 28,  // Wave shape for node 1 (0-5)
    kParam_Node2_WaveShape = 29,  // Wave shape for node 2 (0-5)
    kParam_Node3_WaveShape = 30,  // Wave shape for node 3 (0-5)
    kParam_Node4_WaveShape = 31   // Wave shape for node 4 (0-5)
```

### 6. Add Parameters to Swift Parameter Tree

**Location:** `ModalAttractors/ModalAttractorsExtension/Parameters/Parameters.swift`

**Add new group after "Routing" (after line 138):**

```swift
    // MARK: - Wave Shape Selection
    ParameterGroupSpec(identifier: "waveShapes", name: "Wave Shapes") {
        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Node0_WaveShape,
            identifier: "node0WaveShape",
            name: "Node 0 Wave",
            units: .indexed,
            valueRange: 0...5,
            defaultValue: 0,
            valueStrings: ["Sine", "Sawtooth", "Triangle", "Square", "Pulse 25%", "Pulse 10%"]
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Node1_WaveShape,
            identifier: "node1WaveShape",
            name: "Node 1 Wave",
            units: .indexed,
            valueRange: 0...5,
            defaultValue: 0,
            valueStrings: ["Sine", "Sawtooth", "Triangle", "Square", "Pulse 25%", "Pulse 10%"]
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Node2_WaveShape,
            identifier: "node2WaveShape",
            name: "Node 2 Wave",
            units: .indexed,
            valueRange: 0...5,
            defaultValue: 0,
            valueStrings: ["Sine", "Sawtooth", "Triangle", "Square", "Pulse 25%", "Pulse 10%"]
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Node3_WaveShape,
            identifier: "node3WaveShape",
            name: "Node 3 Wave",
            units: .indexed,
            valueRange: 0...5,
            defaultValue: 0,
            valueStrings: ["Sine", "Sawtooth", "Triangle", "Square", "Pulse 25%", "Pulse 10%"]
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Node4_WaveShape,
            identifier: "node4WaveShape",
            name: "Node 4 Wave",
            units: .indexed,
            valueRange: 0...5,
            defaultValue: 0,
            valueStrings: ["Sine", "Sawtooth", "Triangle", "Square", "Pulse 25%", "Pulse 10%"]
        )
    }
```

### 7. Parameter Handling in SynthEngine

**Location:** `ModalAttractors/ModalAttractorsExtension/DSP/SynthEngine.cpp`

Add parameter handler in `handleParameterEvent()` method:

```cpp
void SynthEngine::handleParameterEvent(const SynthEvent& event) {
    uint32_t paramId = event.parameter.paramId;
    float value = event.parameter.value;

    // ... existing parameter handlers ...

    // NEW: Wave shape parameters
    if (paramId >= kParam_Node0_WaveShape && paramId <= kParam_Node4_WaveShape) {
        uint32_t nodeIndex = paramId - kParam_Node0_WaveShape;
        if (nodeIndex < 5) {
            wave_shape_t shape = static_cast<wave_shape_t>((int)value);
            nodeManager_->setNodeWaveShape(nodeIndex, shape);
        }
    }
}
```

**Add method to NodeManager:**

```cpp
// NodeManager.h
void setNodeWaveShape(uint32_t nodeIndex, wave_shape_t shape);

// NodeManager.cpp
void NodeManager::setNodeWaveShape(uint32_t nodeIndex, wave_shape_t shape) {
    if (nodeIndex >= NUM_NODES) return;

    // Set wave shape for all voices using this node
    for (auto& voice : voices_) {
        if (voice && voice->getNodeIndex() == nodeIndex) {
            modal_node_t* node = voice->getModalNode();
            if (node) {
                node->wave_shape = shape;
            }
        }
    }
}
```

---

## Performance Analysis

### CPU Cost Per Sample (Naive Implementation)

Comparison of waveform generation cost:

| Waveform | Operations | Est. Cycles | vs Sine |
|----------|------------|-------------|---------|
| **Sine** | 1 `sinf()` | ~20-50 | Baseline |
| **Sawtooth** | 1 subtract, 1 divide | ~2-3 | **10x faster** |
| **Triangle** | 1 branch, 3 arithmetic | ~4-5 | **5x faster** |
| **Square/Pulse** | 1 branch, 1 compare | ~3-4 | **7x faster** |

**Key Finding:** Non-sine waveforms are **significantly cheaper** than sine!

**CPU Impact:**
- 20 oscillators currently use `sinf()` = ~400-1000 cycles per sample
- Switching to sawtooth/square = ~60-80 cycles per sample
- **Potential 5-10x performance improvement** for non-sine waves

**Memory Impact:**
- Per-node wave shape: 1 byte × 5 nodes = **5 bytes**
- Per-node pulse width: 4 bytes × 5 nodes = **20 bytes**
- **Total: 25 bytes** (negligible)

### Aliasing Considerations

**The Problem:**
Non-band-limited sawtooth/square waves contain infinite harmonics, which alias above Nyquist frequency.

**Example:** Sawtooth at 880 Hz, 48 kHz sample rate
- Nyquist: 24 kHz
- Harmonics: 880, 1760, 2640, 3520... up to 27,280 Hz (harmonic 31)
- **Harmonics 28+ will alias back as inharmonic noise**

**Mitigation Strategies:**

1. **Naive Implementation (Recommended for Phase 1)**
   - Ship naive oscillators
   - Accept aliasing at high frequencies
   - Users can stay in low-mid range (<880 Hz fundamental)
   - Adds character/grit to sound

2. **Simple Low-Pass Filter (Optional Phase 2)**
   - One-pole LP filter per mode
   - Cutoff at ~4× mode frequency
   - Minimal CPU cost (~10 cycles)

3. **PolyBLEP (Future Phase 3)**
   - Band-limited step functions
   - Excellent quality
   - ~50 extra cycles per discontinuity
   - Reference: https://www.kvraudio.com/forum/viewtopic.php?t=375517

4. **Wavetables (Future Phase 4)**
   - Pre-computed band-limited waveforms
   - Best quality, predictable CPU
   - Memory overhead (~10-50 KB)

**Recommendation:** Start with naive implementation. Users can experiment and decide if aliasing is problematic.

---

## Sonic Possibilities

### What Wave Shapes Bring to Modal Synthesis

**Traditional Synthesis:**
- Static waveform → Envelope → Filter
- Harmonic content is fixed or filtered

**Modal Synthesis with Wave Shapes:**
- **Dynamic waveform** → Modal amplitude evolution → Network coupling
- Harmonic content **evolves with modal decay**
- Each harmonic resonates **at different frequencies** (inharmonic)
- Network coupling creates **harmonic interference patterns**

### Example Sonic Configurations

#### 1. Mixed Spectral Evolution

**Setup:**
- Node 0: Sine (fundamental clarity)
- Node 1: Triangle (warmth/body)
- Node 2: Sawtooth (brightness)
- Node 3: Square (aggressive edge)
- Node 4: Pulse 10% (thin high-frequency shimmer)

**Character:** "Vibrant Bass" → Each node contributes different harmonic flavor
**Result:** Evolving spectral morphing as nodes decay at different rates

#### 2. Resonant Metallic Strike

**Setup:**
- All nodes: Sawtooth wave
- Character: "Metallic Strike" (inharmonic ratios, fast decay)
- High coupling strength

**Result:** Rich metallic resonance with complex beating patterns

#### 3. Organ-like Drone

**Setup:**
- All nodes: Square wave
- Character: "Warm Pad" (harmonic stack 1,2,3,4)
- Personality: Self-Oscillator
- Low damping

**Result:** Classic additive organ sound with modal richness

#### 4. Evolving Pad

**Setup:**
- Node 0-1: Sine (long decay, low frequency)
- Node 2-3: Triangle (medium decay, mid frequency)
- Node 4: Sawtooth (short decay, high frequency)
- Ring topology with medium coupling

**Result:** Pad that evolves from bright → warm → pure over time

#### 5. Percussive Mallet

**Setup:**
- All nodes: Triangle wave
- Character: "Mallet Tone" (wood-like inharmonics)
- High damping
- Personality: Resonator

**Result:** Marimba/vibraphone-like mallet strikes

---

## UI Considerations

### Where to Add Wave Shape Controls

**Option A: Add to Character Editor Tab**
- Wave shape selector next to mode parameters
- Natural location for sound design
- Keeps main UI simple

**Option B: Add New "Wave Shape" Tab**
- Dedicated tab for wave shape selection
- 5 node wave selectors + descriptions
- Educational content about wave shapes

**Option C: Add to Node Selection Area**
- Wave shape selector next to character selector
- Quick access for performance
- Compact layout

**Recommendation:** **Option C** - Place wave selector next to character selector for each node.

**UI Layout Example:**
```
Node 0: [Character: Vibrant Bass ▼] [Wave: Sine ▼]
Node 1: [Character: Dark Node ▼]     [Wave: Triangle ▼]
Node 2: [Character: Bright Bell ▼]   [Wave: Sawtooth ▼]
...
```

### Parameter Automation

All wave shape parameters should be:
- ✅ Automatable (sample-accurate parameter changes)
- ✅ Saved with presets
- ✅ MIDI-learnable (if plugin supports MIDI learn)

**Automation Behavior:**
- Discrete changes (no interpolation between wave shapes)
- Immediate effect (no smoothing)
- Phase-continuous (no phase reset on wave change)

---

## Implementation Checklist

### Phase 1: Core Implementation (~4-6 hours)

- [ ] Add `wave_shape_t` enum to `audio_synth.h`
- [ ] Implement oscillator functions in `audio_synth.c`
- [ ] Add wave_shape field to `modal_node_t` in `modal_node.h`
- [ ] Initialize wave_shape in `modal_node_init()`
- [ ] Replace line 163 in `audio_synth.c` with switch statement
- [ ] Test all 6 waveforms with simple patch

### Phase 2: Parameter Integration (~3-4 hours)

- [ ] Add parameter addresses (kParam_Node0_WaveShape, etc.)
- [ ] Add parameter specs to `Parameters.swift`
- [ ] Implement parameter handler in `SynthEngine.cpp`
- [ ] Add `setNodeWaveShape()` method to `NodeManager`
- [ ] Test parameter automation in DAW

### Phase 3: UI Integration (~2-3 hours)

- [ ] Add wave shape selectors to UI (SwiftUI)
- [ ] Connect UI to parameters
- [ ] Add visual feedback (waveform icons?)
- [ ] Test on macOS and iOS (if applicable)

### Phase 4: Testing & Documentation (~2-3 hours)

- [ ] Create test patches demonstrating wave shapes
- [ ] Document wave shape behavior
- [ ] Test parameter automation
- [ ] Test preset recall
- [ ] Performance profiling (CPU usage)
- [ ] Update user manual

### Phase 5: Character Integration (Optional, ~2-3 hours)

- [ ] Add `mode_wave_shape[4]` to `NodeCharacter`
- [ ] Update all 15 character definitions
- [ ] Test character recall with wave shapes
- [ ] Add wave shape override parameter (if needed)

---

## File Impact Summary

### Files to Modify

| File | Changes | Est. LOC |
|------|---------|----------|
| `DSP/audio_synth.h` | Add enum, helper functions | +30 |
| `DSP/audio_synth.c` | Add oscillators, modify render loop | +80 |
| `DSP/modal_node.h` | Add wave_shape field | +5 |
| `DSP/modal_node.c` | Initialize wave_shape | +3 |
| `Parameters/ModalAttractorsExtensionParameterAddresses.h` | Add addresses | +5 |
| `Parameters/Parameters.swift` | Add parameter specs | +60 |
| `DSP/SynthEngine.cpp` | Add parameter handler | +15 |
| `DSP/NodeManager.h/cpp` | Add setNodeWaveShape method | +20 |

**Total Code Changes:** ~220 lines of code (Phase 1 + 2)

### Files to Create

- (Optional) `DSP/Oscillators.h/cpp` - Separate oscillator library
- (Optional) `UI/WaveShapeSelector.swift` - Reusable wave shape UI component

---

## Risk Assessment

### Technical Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| **Aliasing artifacts** | Medium | Document frequency limits, educate users, add LP filter option |
| **Phase discontinuities on wave change** | Low | Maintain phase accumulator state (already done) |
| **Parameter count explosion** | Low | Use per-node (5 params), not per-mode (20 params) |
| **UI complexity** | Medium | Integrate with character selector, keep simple |
| **Backward compatibility** | Low | Default to WAVE_SINE, existing presets unchanged |

### Musical Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| **Loss of "purity"** | Low | Sine remains default, wave shapes are optional |
| **Harsh timbres** | Low | Users control wave shape per node, can mix |
| **Confusion about purpose** | Medium | Document modal + wave shapes ≠ subtractive synthesis |
| **Preset explosion** | Medium | Update characters to include wave shape suggestions |

**Overall Risk:** **LOW** - This is a low-risk, high-reward feature.

---

## Why This is Exciting

### Unique Sonic Territory

**Modal synthesis + Wave shapes = Unexplored space**

- Traditional modal synthesis (bells, strings): **Pure sine modes only**
- Traditional wave synthesis: **Static harmonic content**
- **This plugin:** Dynamic harmonic evolution through modal network

**What makes it unique:**

1. **Spectral Evolution**
   - Harmonic content changes as modal amplitudes decay
   - Different decay rates = spectral morphing over time

2. **Network Harmonic Coupling**
   - Nodes with sawtooth waves couple with harmonic series
   - Creates complex beating and interference patterns
   - Unpredictable sonic results from network interaction

3. **Inharmonic Richness**
   - Modal frequency ratios (not necessarily harmonic)
   - Combined with harmonic-rich waveforms
   - Creates unique timbres impossible with traditional synthesis

4. **Character + Wave = Infinite Timbres**
   - 15 characters × 6 wave shapes × 5 nodes = **thousands of combinations**
   - Each combination creates distinct evolving timbre

---

## Recommendation

**YES - Implement this feature immediately!**

**Reasons:**

1. ✅ **Perfect architectural fit** - Phase accumulator system ready
2. ✅ **Minimal code impact** - ~220 LOC for full implementation
3. ✅ **Performance improvement** - Non-sine waves are actually faster!
4. ✅ **Unique sonic territory** - Modal + wave shapes is unexplored
5. ✅ **Low risk** - Backward compatible, optional feature
6. ✅ **High user value** - Massively expands sonic palette
7. ✅ **Extensible** - Can add anti-aliasing, per-mode control later

**Suggested Implementation Order:**

1. **Week 1:** Implement Phase 1 + 2 (core + parameters)
2. **Week 2:** Test thoroughly, add UI integration
3. **Week 3:** Document, create demo patches, polish
4. **(Optional) Week 4:** Add wave shape to character system

**Expected Result:** A unique modal synthesis instrument with unprecedented timbral flexibility, combining the best of modal resonance and wave shaping.

---

## Next Steps

1. **Review this document** - Confirm approach and scope
2. **Create implementation branch** - `feature/wave-shape-selector`
3. **Implement Phase 1** - Core oscillators and storage
4. **Test with simple patch** - Verify all wave shapes work
5. **Proceed to Phase 2** - Parameter integration
6. **UI design** - Sketch wave shape selector layout
7. **Full integration testing** - DAW automation, presets, etc.

---

## References

**Xcode Project Files:**
- Audio rendering: `ModalAttractors/ModalAttractorsExtension/DSP/audio_synth.c:163`
- Parameter system: `ModalAttractors/ModalAttractorsExtension/Parameters/Parameters.swift`
- Character definitions: `ModalAttractors/ModalAttractorsExtension/DSP/NodeCharacter.h/cpp`
- Engine integration: `ModalAttractors/ModalAttractorsExtension/DSP/SynthEngine.cpp`

**PolyBLEP Reference:**
- https://www.kvraudio.com/forum/viewtopic.php?t=375517

**Related Documents:**
- `ModalAttractors/ModalAttractorsExtension/DSP/NODE_ARCHITECTURE_REFACTOR.md`
- Original ESP32 exploration: `src/esp32_port/` (for reference only)

---

**Document Version:** 1.0
**Author:** Claude Code Evaluation
**Date:** 2026-01-10
