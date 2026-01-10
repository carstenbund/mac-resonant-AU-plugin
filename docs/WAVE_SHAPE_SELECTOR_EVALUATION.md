# Wave Shape Selector Evaluation

## Executive Summary

**YES** - Adding shaped waves (sawtooth, triangle, square, etc.) to the modal resonator is **highly feasible** and would create fascinating sonic possibilities. The current phase accumulator architecture is perfectly suited for this enhancement.

**Key Insight**: Using non-sinusoidal waveforms in a resonant/modal system creates **spectral evolution** - as the modal amplitudes decay or oscillate, the harmonic content changes dynamically, unlike traditional subtractive synthesis.

---

## Current Architecture Analysis

### How Audio is Generated Now

Location: `src/esp32_port/audio_synth.c:163`

```c
// Current implementation (sine only)
float phase = (phase_acc / 4294967296.0f) * 2.0f * M_PI;
float sample_f = amplitude * sinf(phase);
```

**Architecture strengths:**
- ✅ Phase accumulator system (perfect for any waveform)
- ✅ Phase normalized to [0, 2π) range
- ✅ Clean separation between amplitude (from modal dynamics) and oscillator
- ✅ Per-mode processing (can have different wave shapes per mode)
- ✅ 4 independent oscillators per voice

---

## What Shaped Waves Would Bring

### Sonic Possibilities

1. **Sawtooth Resonators**
   - Rich harmonic content (all harmonics 1/n amplitude)
   - Creates "bright" metallic resonances
   - Modal decay affects entire harmonic spectrum
   - Example: Bell-like tones with complex overtones

2. **Triangle Resonators**
   - Odd harmonics only (1/n² amplitude)
   - Softer than sawtooth, brighter than sine
   - "Hollow" or "woody" character
   - Example: Marimba-like or plucked string tones

3. **Square/Pulse Resonators**
   - Odd harmonics (1/n amplitude)
   - Very aggressive, "reed-like" character
   - Variable pulse width = formant shaping
   - Example: Clarinet-like resonances, aggressive bass

4. **Mixed Mode Timbres**
   - Mode 0: Sine (fundamental clarity)
   - Mode 1: Sawtooth (harmonic richness)
   - Mode 2: Triangle (body/warmth)
   - Mode 3: Pulse (edge/attack)
   - Creates evolving spectral morphing as modes interact

### Why This is Unique

Traditional synthesis: **Static waveform → Filter envelope**

Modal shaped synthesis: **Dynamic amplitude envelope → Shaped oscillator → Spectral evolution**

The modal dynamics create:
- Harmonic content that fades with modal decay
- Frequency-dependent resonance (each harmonic resonates differently)
- Network coupling effects on harmonic structure
- Phase relationships between modes create interference patterns

---

## Implementation Requirements

### 1. Wave Shape Functions

**Location**: `src/esp32_port/audio_synth.c`

Add oscillator functions (phase input: [0, 2π), output: [-1, 1]):

```c
/**
 * @brief Sawtooth wave oscillator
 * @param phase Phase in radians [0, 2π)
 * @return Sample value [-1, 1]
 */
float osc_sawtooth(float phase) {
    // Naive implementation (will alias)
    return 1.0f - (phase / M_PI);
}

/**
 * @brief Triangle wave oscillator
 * @param phase Phase in radians [0, 2π)
 * @return Sample value [-1, 1]
 */
float osc_triangle(float phase) {
    if (phase < M_PI) {
        // Rising edge: -1 to +1
        return -1.0f + (2.0f * phase / M_PI);
    } else {
        // Falling edge: +1 to -1
        return 3.0f - (2.0f * phase / M_PI);
    }
}

/**
 * @brief Square wave oscillator
 * @param phase Phase in radians [0, 2π)
 * @param pulse_width Pulse width [0, 1] (0.5 = square)
 * @return Sample value [-1, 1]
 */
float osc_square(float phase, float pulse_width) {
    float threshold = pulse_width * 2.0f * M_PI;
    return (phase < threshold) ? 1.0f : -1.0f;
}

/**
 * @brief Sine wave (existing)
 */
float osc_sine(float phase) {
    return sinf(phase);
}
```

### 2. Wave Shape Enum

**Location**: `src/esp32_port/audio_synth.h` or `modal_node.h`

```c
/**
 * @brief Oscillator wave shapes
 */
typedef enum {
    WAVE_SHAPE_SINE = 0,      ///< Sine wave (pure fundamental)
    WAVE_SHAPE_SAWTOOTH,      ///< Sawtooth (all harmonics)
    WAVE_SHAPE_TRIANGLE,      ///< Triangle (odd harmonics, soft)
    WAVE_SHAPE_SQUARE,        ///< Square wave (odd harmonics, bright)
    WAVE_SHAPE_PULSE_25,      ///< Pulse 25% duty cycle
    WAVE_SHAPE_PULSE_10,      ///< Pulse 10% duty cycle (thin)
    WAVE_SHAPE_COUNT          ///< Total number of shapes
} wave_shape_t;
```

### 3. Parameter Storage

**Option A: Per-Mode Shape (Recommended)**

Add to `mode_params_t` in `src/esp32_port/modal_node.h:61-67`:

```c
typedef struct {
    float omega;          ///< Angular frequency (rad/s)
    float gamma;          ///< Damping coefficient
    float weight;         ///< Audio contribution weight [0,1]
    wave_shape_t shape;   ///< Oscillator wave shape (NEW)
    float pulse_width;    ///< Pulse width for pulse shapes [0,1] (NEW)
    bool active;          ///< Mode enabled flag
} mode_params_t;
```

**Benefits**:
- Maximum flexibility (each mode can have different timbre)
- Spectral morphing possibilities
- Each mode evolves independently

**Option B: Global Shape**

Add to `audio_synth_params_t` in `src/esp32_port/audio_synth.h:41-47`:

```c
typedef struct {
    float sample_rate;
    uint32_t phase_accumulator[MAX_MODES];
    float mode_gains[MAX_MODES];
    float master_gain;
    wave_shape_t wave_shape;     ///< Global wave shape (NEW)
    float pulse_width;           ///< Global pulse width (NEW)
    bool muted;
} audio_synth_params_t;
```

**Benefits**:
- Simpler implementation
- Easier UI (one control instead of 4)
- Still powerful for most use cases

**Recommendation**: Start with **Option B (global)**, add **Option A (per-mode)** later if needed.

### 4. Render Loop Modification

**Location**: `src/esp32_port/audio_synth.c:163`

**Current code:**
```c
float sample_f = amplitude * sinf(phase);
```

**New code (global shape):**
```c
// Generate sample with selected wave shape
float sample_f;
switch (synth->params.wave_shape) {
    case WAVE_SHAPE_SINE:
        sample_f = amplitude * sinf(phase);
        break;
    case WAVE_SHAPE_SAWTOOTH:
        sample_f = amplitude * osc_sawtooth(phase);
        break;
    case WAVE_SHAPE_TRIANGLE:
        sample_f = amplitude * osc_triangle(phase);
        break;
    case WAVE_SHAPE_SQUARE:
        sample_f = amplitude * osc_square(phase, 0.5f);
        break;
    case WAVE_SHAPE_PULSE_25:
        sample_f = amplitude * osc_square(phase, 0.25f);
        break;
    case WAVE_SHAPE_PULSE_10:
        sample_f = amplitude * osc_square(phase, 0.1f);
        break;
    default:
        sample_f = amplitude * sinf(phase);
}
```

**New code (per-mode shape):**
```c
// Get wave shape for this mode
wave_shape_t shape = node->modes[k].params.shape;
float pulse_width = node->modes[k].params.pulse_width;

// Generate sample with mode-specific wave shape
float sample_f;
switch (shape) {
    case WAVE_SHAPE_SINE:
        sample_f = amplitude * sinf(phase);
        break;
    case WAVE_SHAPE_SAWTOOTH:
        sample_f = amplitude * osc_sawtooth(phase);
        break;
    // ... etc
}
```

### 5. Audio Unit Parameters

**Location**: `src/au_wrapper/ModalParameters.h`

Add parameter IDs:

```cpp
// Global wave shape parameter
kParam_WaveShape = 200,              // Range: 0-5 (enum index)
kParam_PulseWidth = 201,             // Range: 0.01-0.99

// Or per-mode parameters
kParam_Mode0_WaveShape = 200,
kParam_Mode1_WaveShape = 201,
kParam_Mode2_WaveShape = 202,
kParam_Mode3_WaveShape = 203,
kParam_Mode0_PulseWidth = 204,
// ... etc
```

Add parameter definitions in `ModalParameters.cpp`:

```cpp
SetParameter(kParam_WaveShape,
             CFSTR("Wave Shape"),
             kAudioUnitParameterUnit_Indexed,
             0.0f,                    // min (SINE)
             WAVE_SHAPE_COUNT - 1,    // max
             0.0f,                    // default (SINE)
             kAudioUnitParameterFlag_IsReadable |
             kAudioUnitParameterFlag_IsWritable);

SetParameter(kParam_PulseWidth,
             CFSTR("Pulse Width"),
             kAudioUnitParameterUnit_Generic,
             0.01f,                   // min
             0.99f,                   // max
             0.5f,                    // default
             kAudioUnitParameterFlag_IsReadable |
             kAudioUnitParameterFlag_IsWritable);
```

### 6. Parameter Dispatch

**Location**: `src/au_wrapper/ModalAttractorsEngine.cpp`

Add handler in `setParameter()`:

```cpp
case kParam_WaveShape:
    // Set wave shape for all voices
    for (auto& voice : voices_) {
        if (voice) {
            audio_synth_set_wave_shape(&voice->synth_,
                                      static_cast<wave_shape_t>((int)value));
        }
    }
    break;

case kParam_PulseWidth:
    // Set pulse width for all voices
    for (auto& voice : voices_) {
        if (voice) {
            audio_synth_set_pulse_width(&voice->synth_, value);
        }
    }
    break;
```

Add setter functions to `audio_synth.c`:

```c
void audio_synth_set_wave_shape(audio_synth_t* synth, wave_shape_t shape) {
    if (shape >= 0 && shape < WAVE_SHAPE_COUNT) {
        synth->params.wave_shape = shape;
    }
}

void audio_synth_set_pulse_width(audio_synth_t* synth, float width) {
    synth->params.pulse_width = fmaxf(0.01f, fminf(0.99f, width));
}
```

---

## Anti-Aliasing Considerations

### The Aliasing Problem

Non-band-limited sawtooth/square waves contain **infinite harmonics**, which will alias above Nyquist frequency (sample_rate / 2).

**Example at 440 Hz, 48 kHz sample rate:**
- Nyquist: 24 kHz
- Sawtooth harmonics: 440, 880, 1320... **54,560 Hz** (harmonic 124)
- Harmonics 55+ will **alias back** as inharmonic noise

### Solutions (in order of complexity)

#### 1. **Naive Implementation (Start Here)**

**Pros**: Simple, fast, gets the feature working
**Cons**: Audible aliasing at high frequencies

**Recommendation**: Ship naive version first, users can stay in low-mid range or use low-pass filtering.

#### 2. **Low-Pass Filtering**

Add a simple one-pole LP filter after oscillator:

```c
// In audio_synth_t
float lp_state[MAX_MODES];

// In render loop
float cutoff_freq = node->modes[k].params.omega / (2.0f * M_PI);
float alpha = fminf(cutoff_freq * 8.0f / sample_rate, 0.99f);
lp_state[k] += alpha * (sample_f - lp_state[k]);
sample_f = lp_state[k];
```

**Pros**: Very simple, reduces high-frequency content
**Cons**: Dulls the sound, still has some aliasing

#### 3. **PolyBLEP (Best Balance)**

Add band-limited step function to discontinuities.

**Pros**: Excellent quality, reasonable CPU cost
**Cons**: More complex implementation (~50 lines of code)

**Reference**: https://www.kvraudio.com/forum/viewtopic.php?t=375517

#### 4. **Wavetables**

Pre-compute band-limited waveforms at multiple pitch ranges.

**Pros**: Best quality, predictable CPU
**Cons**: Memory overhead, complex implementation

**Recommendation for this project**:
- **Phase 1**: Naive (immediate functionality)
- **Phase 2**: Optional per-mode low-pass filter
- **Phase 3**: PolyBLEP if users demand it

---

## Implementation Phases

### Phase 1: Basic Wave Shapes (2-3 hours)

1. Add wave shape enum and functions to `audio_synth.c`
2. Add global wave_shape parameter to `audio_synth_params_t`
3. Modify render loop with switch statement
4. Add setter function `audio_synth_set_wave_shape()`
5. Test with simple example

**Deliverable**: Working sine/saw/tri/square switching (naive, will alias)

### Phase 2: Audio Unit Integration (2-3 hours)

1. Add AU parameter definitions
2. Wire parameters to ModalAttractorsEngine
3. Test parameter automation
4. Document parameter ranges

**Deliverable**: AU plugin with wave shape control

### Phase 3: Per-Mode Shapes (3-4 hours)

1. Move wave_shape to `mode_params_t`
2. Add per-mode AU parameters (4x controls)
3. Update UI to handle multiple selectors
4. Add presets showcasing mixed timbres

**Deliverable**: Independent wave shape per mode

### Phase 4: Anti-Aliasing (Optional, 4-6 hours)

1. Implement PolyBLEP for sawtooth/square
2. Add optional LP filter parameter
3. Frequency-dependent quality selection
4. CPU benchmarking

**Deliverable**: Professional-quality band-limited oscillators

### Phase 5: Advanced Shapes (Optional, 2-3 hours)

1. Add more waveforms (noise, sub-octave, formant)
2. Implement variable pulse width parameter
3. Add wave morphing (interpolate between shapes)

**Deliverable**: Extended palette of timbres

---

## Code Impact Summary

### Files to Modify

| File | Changes | LOC |
|------|---------|-----|
| `src/esp32_port/audio_synth.h` | Add enum, parameter field | +20 |
| `src/esp32_port/audio_synth.c` | Add oscillator functions, modify render | +80 |
| `src/au_wrapper/ModalParameters.h` | Add parameter IDs | +5 |
| `src/au_wrapper/ModalParameters.cpp` | Add parameter definitions | +30 |
| `src/au_wrapper/ModalAttractorsEngine.cpp` | Add parameter dispatch | +20 |

**Total estimated LOC**: ~155 lines (Phase 1 + 2)

### Performance Impact

**CPU cost per sample (naive implementation)**:
- Sine: 1 `sinf()` call (~20 cycles)
- Sawtooth: 2 arithmetic ops (~2 cycles) **10x faster!**
- Triangle: 1 branch + 3 arithmetic ops (~4 cycles) **5x faster!**
- Square: 1 branch + 1 compare (~3 cycles) **7x faster!**

**Conclusion**: Non-sine waveforms are **CHEAPER** than sine! (unless using PolyBLEP)

### Memory Impact

- Enum: 1 byte per mode or synth
- Pulse width: 4 bytes (float)
- Total per voice: 5 bytes (global) or 20 bytes (per-mode)

**Negligible** memory impact.

---

## Risk Assessment

### Technical Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| Aliasing artifacts | Medium | Document frequency limits, add LP filter option |
| CPU overhead (PolyBLEP) | Low | Keep naive version as default, PolyBLEP optional |
| UI complexity (per-mode) | Medium | Start with global parameter, add per-mode later |
| Parameter explosion | Low | Group parameters logically, use sensible defaults |

### Musical Risks

| Risk | Severity | Notes |
|------|----------|-------|
| Overly harsh tones | Low | Users can mix sine modes for smoothness |
| Loss of "purity" | Low | Sine remains default, optional feature |
| Confusion with purpose | Medium | Document that this is **modal shaping**, not subtractive synthesis |

**Overall risk**: **LOW** - This is a low-risk, high-reward feature.

---

## Sonic Examples to Explore

Once implemented, try these combinations:

1. **Metallic Bell**
   - Mode 0: Sine (fundamental clarity)
   - Mode 1: Triangle (odd harmonics body)
   - Mode 2-3: Sawtooth (bright shimmer)
   - High damping (short decay)

2. **Drone Synth**
   - All modes: Square wave
   - Self-oscillator personality
   - Low damping
   - High coupling strength

3. **Plucked String**
   - Mode 0: Sawtooth (attack brightness)
   - Mode 1-3: Triangle (warm decay)
   - Resonator personality
   - Medium damping

4. **Evolving Pad**
   - Mode 0: Sine (low freq, long decay)
   - Mode 1: Triangle (mid freq, medium decay)
   - Mode 2: Sawtooth (high freq, fast decay)
   - Mode 3: Pulse 10% (very high freq, very fast decay)
   - Creates spectral evolution from bright → warm → pure

---

## Recommendation

**YES - Implement this feature!**

**Recommended path:**
1. Start with **Phase 1** (global wave shape, naive implementation)
2. Test sonic possibilities
3. Add **Phase 2** (AU parameters)
4. If users love it → **Phase 3** (per-mode shapes)
5. If aliasing is problematic → **Phase 4** (anti-aliasing)

**Why this is exciting:**
- Leverages existing phase accumulator perfectly
- Minimal code changes (~155 LOC)
- Actually **improves** CPU performance (non-sine is cheaper!)
- Creates unique sonic territory (modal + shaped waves)
- Low risk, high musical reward

The combination of **modal decay dynamics** with **harmonic-rich waveforms** creates sounds impossible with traditional synthesis. This is **spectral resonance**, not just oscillator selection.

---

## References

- Current sine oscillator: `src/esp32_port/audio_synth.c:163`
- Modal dynamics: `src/esp32_port/modal_node.c:121-187`
- Phase accumulator: `src/esp32_port/audio_synth.c:156-170`
- Parameter system: `src/au_wrapper/ModalParameters.h`

**Next steps**: Review this document, decide on implementation scope, create tasks.
