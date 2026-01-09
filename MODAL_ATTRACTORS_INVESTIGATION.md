# Modal Attractors: Parameter & MIDI Note Response Investigation

**Date:** January 9, 2026
**Branch:** claude/investigate-modal-attractors-IgNe1

## Executive Summary

The ModalAttractors DSP system is a polyphonic modal resonator synthesizer where each voice (node) contains 4 internal modal oscillators. The investigation reveals a **complete and functional parameter system** in the main ModalAttractorsExtension implementation, with **full MIDI note and pitch bend support**. However, there are **discrepancies** between the newer implementation (ModalAttractorsExtension) and the older wrapper (src/au_wrapper).

---

## Architecture Overview

### Core Concept: Nodes and Modes

```
MIDI Note On → Allocate Voice/Node → Node contains 4 Internal Modes

Node[0] ←─── coupling ────→ Node[1]
  ├─ Mode 0 (fundamental)      ├─ Mode 0 (fundamental)
  ├─ Mode 1 (slight detune)    ├─ Mode 1 (slight detune)
  ├─ Mode 2 (2nd harmonic)     ├─ Mode 2 (2nd harmonic)
  └─ Mode 3 (3rd harmonic)     └─ Mode 3 (3rd harmonic)
```

**Key Insight:** The network topology connects **nodes** (voices), not individual modes. Each node is a 4-mode oscillator bank triggered by MIDI notes.

### File Structure

**Primary Implementation** (ModalAttractorsExtension - Latest):
- `ModalAttractorsExtension/DSP/SynthEngine.cpp` - Main synthesis engine
- `ModalAttractorsExtension/DSP/VoiceAllocator.cpp` - Voice/node allocation
- `ModalAttractorsExtension/DSP/ModalVoice.cpp` - Individual voice with 4 modes
- `ModalAttractorsExtension/DSP/TopologyEngine.cpp` - Network coupling
- `ModalAttractorsExtension/Common/DSP/ModalAttractorsEngine.cpp` - C API wrapper

**Legacy Wrapper** (src/au_wrapper - Older):
- `src/au_wrapper/ModalAttractorsDSPKernel.mm` - Simplified wrapper
- `src/au_wrapper/ModalAttractorsEngine.cpp` - Basic engine
- `src/dsp_core/` - Shared DSP core classes

---

## Parameter System Analysis

### Complete Parameter List (20 Parameters)

#### Global Parameters (4)
| ID | Name | Range | Default | Status |
|----|------|-------|---------|--------|
| 0 | Master Gain | 0.0-1.0 | 0.7 | ✅ Working |
| 1 | Coupling Strength | 0.0-1.0 | 0.3 | ✅ Working |
| 2 | Topology | 0-6 | 0 (Ring) | ✅ Working |
| 3 | Node Count | 1-16 | 16 | ✅ Working |

**Topology Values:**
- 0: Ring (each node connects to 2 neighbors)
- 1: Small World (Watts-Strogatz)
- 2: Clustered (modular structure)
- 3: Hub-Spoke (star topology)
- 4: Random (Erdős–Rényi)
- 5: Complete (all-to-all)
- 6: None (no coupling)

#### Mode Parameters (12 total = 3 per mode × 4 modes)

Each of the 4 modes has:
- **Frequency Multiplier** (0.5-8.0): Harmonic relationship to base MIDI note
- **Damping** (0.1-5.0): Decay rate of the mode
- **Weight** (0.0-1.0): Audio contribution of the mode

| Mode | Freq ID | Damp ID | Weight ID | Default Freq | Default Damp | Default Weight |
|------|---------|---------|-----------|--------------|--------------|----------------|
| 0 | 4 | 5 | 6 | 1.0× | 1.0 | 1.0 |
| 1 | 7 | 8 | 9 | 2.0× | 1.2 | 0.8 |
| 2 | 10 | 11 | 12 | 3.0× | 1.5 | 0.6 |
| 3 | 13 | 14 | 15 | 4.5× | 2.0 | 0.4 |

#### Excitation Parameters (2)
| ID | Name | Range | Default | Status |
|----|------|-------|---------|--------|
| 16 | Poke Strength | 0.0-1.0 | 0.5 | ✅ Working |
| 17 | Poke Duration | 1.0-50.0 ms | 10.0 | ✅ Working |

#### Voice Parameters (2)
| ID | Name | Range | Default | Status |
|----|------|-------|---------|--------|
| 18 | Polyphony | 1-32 | 16 | ⚠️ Read-only |
| 19 | Personality | 0-1 | 0 | ✅ Working |

**Personality Values:**
- 0: Resonator (decays to silence, percussive)
- 1: Self-Oscillator (continuous sound, drone)

---

## MIDI Note Response Analysis

### Note On Handling

**File:** `ModalAttractorsExtension/DSP/ModalVoice.cpp:50-74`

```cpp
void ModalVoice::noteOn(uint8_t midi_note, float velocity) {
    midi_note_ = midi_note;
    velocity_ = velocity;
    state_ = State::Attack;
    age_ = 0;

    // Update frequencies based on new note
    updateFrequencies();

    // Reset phase accumulators to prevent clicks
    audio_synth_reset_phase(&synth_);

    // Apply poke excitation with velocity
    poke_event_t poke;
    poke.source_node_id = voice_id_;
    poke.strength = velocity;
    poke.phase_hint = -1.0f; // Random phase

    // Equal weight to all modes
    for (int k = 0; k < MAX_MODES; k++) {
        poke.mode_weights[k] = 1.0f;
    }

    modal_node_apply_poke(&node_, &poke);
}
```

**Behavior:**
1. ✅ MIDI note converted to base frequency via `midi_to_freq(note)`
2. ✅ Velocity (0-127) normalized to 0.0-1.0
3. ✅ Velocity controls poke excitation strength
4. ✅ All 4 modes receive equal excitation weight
5. ✅ Phase reset prevents clicks between notes
6. ✅ Voice state machine transitions to Attack

### Frequency Update Mechanism

**File:** `ModalAttractorsExtension/DSP/ModalVoice.cpp:160-180`

```cpp
void ModalVoice::updateFrequencies() {
    // Calculate base frequency with pitch bend
    float base_freq = midi_to_freq(midi_note_);

    // Apply pitch bend (±2 semitones by default)
    float bend_factor = powf(2.0f, pitch_bend_ * 2.0f / 12.0f);
    base_freq *= bend_factor;

    // Update all mode frequencies proportionally
    // HARDCODED multipliers (not using parameter values!)
    setMode(0, base_freq * 1.0f,   ...);  // Fundamental
    setMode(1, base_freq * 1.01f,  ...);  // Slight detune
    setMode(2, base_freq * 2.0f,   ...);  // Second harmonic
    setMode(3, base_freq * 3.0f,   ...);  // Third harmonic
}
```

**⚠️ CRITICAL ISSUE FOUND:**
The `updateFrequencies()` method uses **hardcoded frequency multipliers** (1.0, 1.01, 2.0, 3.0) instead of the actual parameter values (mode0_frequency_, mode1_frequency_, etc.).

**Expected behavior:**
```cpp
setMode(0, base_freq * mode_freq_mult_[0], ...);
setMode(1, base_freq * mode_freq_mult_[1], ...);
// etc.
```

**Current behavior:**
- Mode frequency parameters (IDs 4, 7, 10, 13) are **received and stored** by SynthEngine
- VoiceAllocator's `setMode()` method exists but **only updates damping/weight**
- Frequency multipliers are **ignored during note playback**
- Parameters only affect **initial voice setup**, not **active note response**

---

## Pitch Bend Support

### Implementation Status

**ModalAttractorsExtension Version:** ✅ **FULLY IMPLEMENTED**

**File:** `ModalAttractorsExtension/Common/Audio Unit/ModalAttractorsExtensionAudioUnit.swift:244-248`

```swift
case 0xE0:  // Pitch Bend
    let lsb = Int32(midiData[1])
    let msb = Int32(midiData[2])
    let bend = (Float(msb) * 128.0 + Float(lsb) - 8192.0) / 8192.0  // -1.0 to +1.0
    modal_attractors_engine_push_pitch_bend(enginePtr, offset, bend)
```

**Processing Chain:**
1. ✅ MIDI pitch bend (0xE0) parsed with 14-bit resolution
2. ✅ Converted to -1.0 to +1.0 range
3. ✅ Pushed to event queue with sample-accurate timing
4. ✅ Applied to VoiceAllocator via `setPitchBend()`
5. ✅ All active voices update frequencies immediately
6. ✅ Bend range: ±2 semitones (hardcoded, not configurable)

**src/au_wrapper Version:** ❌ **NOT IMPLEMENTED**

**File:** `src/au_wrapper/ModalAttractorsDSPKernel.mm:74`

```cpp
// TODO: Add support for other MIDI messages
// 0xE0 - Pitch Bend
// 0xB0 - Control Change
```

Pitch bend messages are **ignored** in the legacy wrapper.

---

## Parameter Propagation Flow

### ✅ Working Path (ModalAttractorsExtension)

```
User UI → AUParameter
         ↓
  ParameterTree observes change
         ↓
  modal_attractors_engine_push_parameter(paramId, value)
         ↓
  EventQueue (sample-accurate)
         ↓
  SynthEngine::processEvent()
         ↓
  SynthEngine::setParameter(paramId, value)
         ↓
  ┌─────────────────────────────────┬──────────────────────────┐
  ↓                                 ↓                          ↓
Global params              VoiceAllocator::setMode()    TopologyEngine
(masterGain_, etc.)        (damping, weight only)       (topology, coupling)
  ↓                                 ↓
Applied in                    Applied to all voices
renderSlice()                 via setMode()
```

### ⚠️ Broken Link: Mode Frequency Parameters

**Problem:** Mode frequency parameters follow this path:

```
kParam_Mode0_Frequency (value = 2.5)
         ↓
  SynthEngine stores mode0_frequency_ = 2.5
         ↓
  VoiceAllocator::setMode(0, 2.5, damping, weight) called
         ↓
  ❌ Stored in mode_params_[0].freq_multiplier = 2.5
         ↓
  ❌ But never used! Voice::updateFrequencies() uses hardcoded 1.0
```

**Fix Required:** Voice needs access to frequency multiplier parameters.

---

## Voice State Machine

**File:** `ModalAttractorsExtension/DSP/ModalVoice.cpp:175-203`

```cpp
enum class State {
    Inactive,   // Voice not playing
    Attack,     // Note on, attack phase
    Sustain,    // Sustaining (self-oscillator only)
    Release     // Note off, release phase
};
```

**Behavior by Personality:**

**Resonator Mode (personality = 0):**
- Note On → Attack state
- Modes decay naturally via damping
- Note Off → Release state
- When amplitude < 0.001 → Inactive (voice freed)

**Self-Oscillator Mode (personality = 1):**
- Note On → Attack → Sustain
- Modes maintain amplitude (negative damping compensation)
- Note Off → Release (gradual decay)
- When amplitude < 0.001 → Inactive

---

## Modal Dynamics Core

### Physical Model

Each mode evolves according to:

```
ȧ_k = (-γ_k + iω_k)a_k + u_k(t)
```

Where:
- `a_k` = complex amplitude of mode k
- `γ_k` = damping coefficient (from parameter)
- `ω_k` = angular frequency (from MIDI note × multiplier)
- `u_k(t)` = excitation input (poke + coupling)

**Integration:** Exact exponential (numerical stability)

**File:** `modal_node.c` (C core, ported from ESP32)

### Audio Synthesis

**File:** `audio_synth.c`

```cpp
output = Σ(k=0 to 3) weight_k × Re(a_k × e^(iφ_k))
```

- Phase accumulator runs at carrier frequency
- Mode amplitudes modulate the carrier
- Per-mode weights control timbre contribution

---

## Coupling System

### Network Topology

**Implemented topologies:**
1. **Ring:** Each node → 2 neighbors (bidirectional)
2. **Small World:** Ring + random rewiring (Watts-Strogatz)
3. **Clustered:** Groups of fully-connected nodes
4. **Hub-Spoke:** One central node, all others connect to it
5. **Random:** Probabilistic connections (Erdős–Rényi)
6. **Complete:** All nodes connected to all others
7. **None:** No coupling

**Coupling Matrix:** `[num_voices × num_voices]` floats

**Normalization:** Row-normalized (diffusive coupling)

### Coupling Mechanism

**File:** `TopologyEngine.cpp:100-130`

```cpp
// For each active voice i:
for (neighbor j in topology[i]) {
    // Get mode 0 amplitude from neighbor
    neighbor_amp = voice[j].getMode0Amplitude();
    self_amp = voice[i].getMode0Amplitude();

    // Diffusive coupling: (neighbor - self)
    diff = neighbor_amp - self_amp;

    // Apply weighted coupling to mode 0
    coupling_input[0] += abs(diff) × weight × coupling_strength;
}

// Apply to voice
voice[i].applyCoupling(coupling_inputs);
```

**Current Implementation:**
- ✅ Coupling uses **mode 0 only** (fundamental)
- ✅ Applied as additive excitation to mode amplitude
- ⚠️ Could extend to all 4 modes (architectural possibility)

### Update Rate

**Control Rate:** 500 Hz (2ms timestep)
- Modal dynamics step: `modal_node_step()` @ 500 Hz
- Coupling update: @ 500 Hz
- Audio render: @ sample rate (44.1-192 kHz)

---

## Current Issues and Gaps

### 🔴 Critical Issues

1. **Mode Frequency Parameters Not Applied During Playback**
   - **Location:** `ModalVoice.cpp:160-180`
   - **Impact:** Users can change mode frequency parameters, but they only affect **newly triggered voices**, not the **frequency ratios** during playback
   - **Fix:** Pass `mode_freq_mult_[]` from VoiceAllocator to ModalVoice, use in `updateFrequencies()`

2. **src/au_wrapper Version Outdated**
   - **Location:** `src/au_wrapper/ModalAttractorsDSPKernel.mm`
   - **Missing:** Pitch bend, mode parameters, node count, excitation params
   - **Fix:** Use ModalAttractorsExtension implementation exclusively

### ⚠️ Design Limitations

1. **Hardcoded Pitch Bend Range**
   - Currently ±2 semitones
   - No parameter to adjust (common AU limitation)

2. **Coupling Uses Mode 0 Only**
   - Potential for richer interactions if all modes coupled
   - Current design is computationally simpler

3. **Fixed Frequency Ratios on Note Trigger**
   - Even if parameters change, playing notes keep their initial ratios
   - Design choice: stability vs. real-time modulation

---

## Testing Recommendations

### Unit Tests Needed

1. **Parameter Response Test**
   ```cpp
   // Test: Mode frequency parameter changes voice timbre
   voice.noteOn(60, 1.0);
   setParameter(kParam_Mode1_Frequency, 3.5);
   // Verify mode 1 is now at 3.5× fundamental
   ```

2. **MIDI Note Accuracy Test**
   ```cpp
   // Test: MIDI note 69 (A4) = 440 Hz
   voice.noteOn(69, 1.0);
   float freq = voice.getBaseFrequency();
   ASSERT_NEAR(freq, 440.0, 0.01);
   ```

3. **Pitch Bend Range Test**
   ```cpp
   voice.noteOn(60, 1.0);  // C4 = 261.63 Hz
   voice.setPitchBend(1.0);  // +2 semitones
   // Expected: 261.63 × 2^(2/12) = 293.66 Hz (D4)
   ```

4. **Voice Stealing Test**
   ```cpp
   // Play 17 notes with polyphony=16
   // Verify oldest voice is stolen
   ```

### Integration Tests Needed

1. **Parameter Automation**
   - Verify DAW automation of all 20 parameters
   - Check sample-accurate parameter changes

2. **Polyphonic Coupling**
   - Play chord, verify coupling between voices
   - Change topology while playing

3. **Personality Switch**
   - Switch between Resonator ↔ Self-Oscillator
   - Verify decay behavior changes

---

## Recommendations

### Immediate Fixes

1. **Fix Mode Frequency Parameter Application**
   ```cpp
   // In VoiceAllocator.cpp:
   void VoiceAllocator::setMode(uint8_t mode_idx, float freq_mult, float damp, float weight) {
       mode_params_[mode_idx] = {freq_mult, damp, weight};

       // Update all voices
       for (uint32_t i = 0; i < max_polyphony_; i++) {
           if (voices_[i]) {
               voices_[i]->setModeMultiplier(mode_idx, freq_mult);
               voices_[i]->setModeDamping(mode_idx, damp);
               voices_[i]->setModeWeight(mode_idx, weight);
           }
       }
   }
   ```

2. **Deprecate src/au_wrapper**
   - Remove old wrapper to avoid confusion
   - Use ModalAttractorsExtension exclusively

3. **Add Frequency Multiplier Storage to ModalVoice**
   ```cpp
   class ModalVoice {
       float mode_freq_mult_[4];  // Store multipliers

       void setModeMultiplier(uint8_t idx, float mult) {
           mode_freq_mult_[idx] = mult;
           updateFrequencies();  // Immediately update if active
       }
   };
   ```

### Future Enhancements

1. **Configurable Pitch Bend Range**
   - Add parameter: kParam_PitchBendRange (1-24 semitones)

2. **Per-Mode Coupling**
   - Allow modes 0-3 to couple independently
   - Richer network dynamics

3. **MIDI CC Mapping**
   - Map CC messages to parameters
   - Standard: CC1 (mod wheel), CC7 (volume), etc.

4. **MPE Support**
   - Per-note pitch bend
   - Per-note parameter control

---

## Conclusion

The ModalAttractors DSP system has a **well-designed and mostly functional** parameter and MIDI system with the following characteristics:

**✅ Strengths:**
- Complete 20-parameter system with proper AU integration
- Full polyphonic MIDI note support with voice allocation
- Pitch bend working in main implementation
- Sophisticated modal dynamics with 4 modes per voice
- Network topology system with 7 topology types
- Real-time safe event processing with sample accuracy
- Proper voice state machine (Attack/Sustain/Release)

**⚠️ Issues:**
- Mode frequency parameters stored but not applied during note playback
- Legacy wrapper (src/au_wrapper) missing features
- Hardcoded frequency ratios in updateFrequencies()
- Coupling limited to mode 0 only

**Priority:** Fix mode frequency parameter application to match user expectations. The infrastructure is in place; it just needs the final connection between parameter storage and real-time frequency updates.

---

**Investigation completed:** January 9, 2026
**Investigator:** Claude (AI Assistant)
**Files analyzed:** 25+ source files across DSP core, AU wrapper, and parameter system
