# ModalAttractor Resonant Body Effect

**Status:** ✅ Core DSP Implementation Complete
**Date:** 2026-01-11
**Branch:** `claude/modal-attractor-effect-1eLxF`

## Overview

This document describes the **ModalAttractor Resonant Body Effect** - a stateful, physical resonator effect that processes audio input through a modal synthesis engine.

### Mental Model

This is **NOT**:
- A resonant filter (linear, memoryless)
- A pitch tracker / follower
- A vocoder
- A traditional audio effect

This **IS**:
- A physical resonant body attached to the signal path
- Like sympathetic strings on a guitar
- Like a piano soundboard reacting to a violin
- Like a metal plate bolted onto a drum
- Like a spring reverb (but the resonator itself, before the reverb exists)

**Key principle:** Input audio **injects energy** into the resonator. The resonator has its own modal structure, decay characteristics, and pitch tendencies. It keeps ringing after the input stops.

---

## Architecture

### Signal Flow

```
Audio Input (stereo)
   ↓
[Energy Extractor] ──► Broadband RMS (attack/release envelope)
   ↓
[Spectral Analyzer] ──► 3-Band RMS (Low/Mid/High)
   ↓
[Pitch Detector] ──► Dominant frequency (optional, for morph)
   ↓
[Excitation Mapper] ──► Distributes energy to modal weights
   ↓
[Modal Resonators] ──► 4-mode resonant body (stateful)
   ↓
[Audio Synthesis] ──► Renders modal oscillators
   ↓
[Dry/Wet Mix] ──► Output (stereo)
```

### Core Components

All implemented in `ModalAttractors/DSP/`:

1. **EnergyExtractor** (`EnergyExtractor.h/cpp`)
   - RMS computation with attack/release smoothing
   - Attack: ~5ms (fast response to transients)
   - Release: ~100ms (maintains sustain)
   - Output: 0.0-1.0 energy level

2. **SpectralAnalyzer** (`SpectralAnalyzer.h/cpp`)
   - 3-band biquad filter bank
   - Low: 20 Hz - 400 Hz (bass, fundamentals)
   - Mid: 400 Hz - 3 kHz (body, harmonics)
   - High: 3 kHz - 20 kHz (brightness, transients)
   - Per-band RMS for modal weight distribution

3. **PitchDetector** (`PitchDetector.h/cpp`)
   - Autocorrelation-based pitch detection
   - Range: 60 Hz - 2000 Hz (B1 to B6)
   - Updates slowly (~40ms averaging)
   - Only active when Morph > 0

4. **ResonantBodyProcessor** (`ResonantBodyProcessor.h/cpp`)
   - Main effect processor
   - Integrates all analysis components
   - Drives modal resonators (reuses existing `modal_node_t` infrastructure)
   - Renders audio and mixes with dry signal

---

## User Parameters (Effect-Style)

| Parameter | Range | Description | Internal Mapping |
|-----------|-------|-------------|------------------|
| **Body Size** | 0.0 - 1.0 | Size of resonant body | Frequency scale: 4x (small) to 0.25x (large) |
| **Material** | 0.0 - 1.0 | Material hardness | Damping: 5.0 (soft) to 0.1 (hard) |
| **Excite** | 0.0 - 1.0 | Input coupling strength | Energy injection gain |
| **Morph** | 0.0 - 1.0 | Pitch tracking flexibility | Frequency drift rate toward input |
| **Mix** | 0.0 - 1.0 | Dry/wet balance | 0 = dry only, 1 = wet only |

### Parameter Behaviors

- **Body Size = 0.0:** Very small body, high pitch (4x frequency), like a tiny bell
- **Body Size = 0.5:** Medium body, normal pitch (1x frequency)
- **Body Size = 1.0:** Very large body, low pitch (0.25x frequency), like a large drum

- **Material = 0.0:** Soft, lossy (high damping, short decay), like felt
- **Material = 0.5:** Balanced (moderate decay)
- **Material = 1.0:** Hard, resonant (low damping, long ring), like metal

- **Excite = 0.0:** No excitation (resonator silent)
- **Excite = 1.0:** Maximum excitation (strong coupling)

- **Morph = 0.0:** Fixed tuning (resonator never changes pitch)
- **Morph = 1.0:** Maximum morphing (tracks input pitch, but slowly)

- **Mix = 0.0:** Dry signal only (bypass)
- **Mix = 1.0:** Wet signal only (resonator response)

---

## Implementation Details

### Modal Resonator Physics

Each resonator has **4 complex modes** evolving according to:

```
ȧₖ = (-γₖ + iωₖ)aₖ + uₖ(t)
```

Where:
- `aₖ` = complex amplitude of mode k
- `ωₖ` = angular frequency (rad/s)
- `γₖ` = damping coefficient
- `uₖ(t)` = excitation input (from audio energy)

**Default Mode Configuration (Harmonic Series):**
- Mode 0: 1.0x base frequency (fundamental)
- Mode 1: 2.0x base frequency (octave)
- Mode 2: 3.0x base frequency (perfect fifth above octave)
- Mode 3: 5.0x base frequency (two octaves + major third)

**Alternative (Inharmonic - Bell-like):**
- Mode 0: 1.0x
- Mode 1: 2.76x
- Mode 2: 5.40x
- Mode 3: 8.93x

### Energy Injection Strategy

Spectral content weights mode excitation:

```
Mode 0 (fundamental):   0.7 * low_energy + 0.3 * mid_energy
Mode 1 (1st harmonic):  0.3 * low_energy + 0.5 * mid_energy
Mode 2 (2nd harmonic):  0.2 * mid_energy + 0.6 * high_energy
Mode 3 (3rd harmonic):  0.4 * high_energy
```

This creates spectrally-aware resonance: bass-heavy inputs excite low modes, treble-heavy inputs excite high modes.

### Pitch Morphing (Optional)

When `Morph > 0`, the resonator's base frequency slowly drifts toward the detected input pitch:

```
base_frequency += morph_rate * (detected_pitch - base_frequency)
```

- Morph rate: ~0.01 per control update (200 Hz)
- Full convergence: ~0.5 seconds at Morph = 1.0
- Only active when pitch confidence > 0.3

This creates the effect of a resonant body that "tunes itself" to the instrument playing into it.

---

## Testing

### Build & Run Tests

```bash
./build_resonant_body_test.sh
cd build_resonant_body
./test_resonant_body
```

### Test Outputs

Four WAV files are generated demonstrating different behaviors:

1. **test_resonant_body_impulse.wav**
   - Impulse train at 4 Hz
   - Listen for: Resonant ringing, decay characteristics
   - Expected: Each impulse excites resonators, which ring and decay

2. **test_resonant_body_sweep.wav**
   - Sine sweep 50 Hz → 2000 Hz
   - Listen for: Resonant peaks at mode frequencies
   - Expected: Strong response at ~220 Hz, 440 Hz, 660 Hz, 1100 Hz

3. **test_resonant_body_noise.wav**
   - White noise excitation
   - Listen for: Resonator acting as spectral filter
   - Expected: Noise colored by resonator's frequency response

4. **test_resonant_body_morph.wav**
   - Sine sweep with morphing enabled (Morph = 0.5)
   - Listen for: Resonator slowly tracking input pitch
   - Expected: Resonant frequencies drift toward sweep frequency

---

## Next Steps

### Phase 1: Integration with AU Framework ⏳

The DSP is complete. Now we need to integrate it into an Audio Unit plugin.

**Two approaches:**

#### **Option A: New AU Effect Plugin (Recommended)**
- Create `ModalAttractorEffect` as separate AU
- Type: **AU Effect** (processes audio input/output)
- Advantages:
  - Clean separation from synth
  - Proper AU Effect semantics
  - Can process any audio source
- Implementation:
  - Create new Xcode AU Extension target
  - Wrap `ResonantBodyProcessor` in AU render callback
  - Expose 5 parameters (Body Size, Material, Excite, Morph, Mix)

#### **Option B: Extend Existing AU with Mode Switch**
- Add "Mode" parameter: Synth vs Effect
- Route audio input when in Effect mode
- Advantages:
  - Single plugin
- Disadvantages:
  - More complex routing
  - AU Instrument vs AU Effect type mismatch

**Recommendation:** **Option A** for clarity and proper AU architecture.

### Phase 2: Parameter UI Design 🎨

Design intuitive controls for the 5 parameters:
- Sliders or rotary knobs
- Visual feedback of resonator energy
- Spectrum analyzer showing resonant peaks (optional)

### Phase 3: Preset System 💾

Create factory presets demonstrating different use cases:
- "Sympathetic Strings" - guitar/string resonance
- "Drum Shell" - percussive body
- "Metal Plate" - bright, metallic
- "Wooden Box" - warm, mellow
- "Glass Bell" - inharmonic, shimmer
- "Morphing Body" - pitch-tracking enabled

### Phase 4: Testing & Validation ✅

- Test with various input sources (drums, guitar, vocals, synth)
- Validate parameter ranges (ensure no instability)
- CPU profiling (should be very efficient)
- A/B comparison with traditional resonant filters

---

## File Structure

```
ModalAttractors/
├── DSP/
│   ├── EnergyExtractor.h/cpp          ← RMS energy extraction
│   ├── SpectralAnalyzer.h/cpp         ← 3-band spectral analysis
│   ├── PitchDetector.h/cpp            ← Autocorrelation pitch detector
│   └── ResonantBodyProcessor.h/cpp    ← Main effect processor
│
Tests/
└── test_resonant_body.cpp             ← Standalone DSP test

build_resonant_body_test.sh            ← Build script for test
build_resonant_body/
├── test_resonant_body                 ← Test executable
├── test_resonant_body_impulse.wav     ← Test output 1
├── test_resonant_body_sweep.wav       ← Test output 2
├── test_resonant_body_noise.wav       ← Test output 3
└── test_resonant_body_morph.wav       ← Test output 4
```

---

## Technical Specifications

| Specification | Value |
|---------------|-------|
| **Sample Rates** | 44.1, 48, 88.2, 96 kHz (any) |
| **Latency** | ~5ms (control rate updates) |
| **CPU Usage** | <1% per instance @ 48 kHz (estimated) |
| **Memory** | ~100 KB per instance |
| **Modes per Instance** | 4 complex modes |
| **Channels** | Stereo in/out |
| **Precision** | 32-bit float |

---

## Design Philosophy

This effect embodies the principle:

> **"The input does not control the sound. The input injects energy into a resonant system."**

This is why:
- The resonator keeps ringing after input stops (stores energy)
- The resonator has its own pitch tendencies (autonomous)
- Spectral content matters (different frequencies excite different modes)
- Morphing is slow (inertia, like temperature changes)

It's not a filter. It's not a synthesizer. It's a **physical resonant body** that responds to the world around it.

---

## Credits

- **Architecture & DSP:** Based on modal synthesis principles
- **Physics Engine:** Ported from ESP32 distributed resonator system
- **Implementation:** Claude on 2026-01-11

---

## References

- Modal synthesis: Fletcher & Rossing, "The Physics of Musical Instruments"
- Complex mode decomposition: Chaigne & Kergomard, "Acoustics of Musical Instruments"
- Existing codebase: `ModalAttractors` AU Plugin (synthesizer version)

---

## Questions?

For implementation questions, see:
- `ResonantBodyProcessor.h` - Main API documentation
- `test_resonant_body.cpp` - Usage examples
- Existing modal infrastructure in `ModalAttractorsExtension/DSP/`
