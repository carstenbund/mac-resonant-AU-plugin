# Hybrid Waveform DSP Implementation

## Overview

This document describes the physically-correct hybrid approach to waveform selection in the Modal Attractors DSP engine.

## Physical Model Background

The modal attractor equations are based on sinusoidal eigenmodes:

```
ȧₖ = (-γₖ + iωₖ)aₖ + driveₖ + couplingₖ
```

Each complex coefficient `aₖ` represents the amplitude and phase of a **sinusoidal** eigenmode at frequency `ωₖ`:

```
yₖ(t) = ℜ(aₖ(t)) = |aₖ|sin(φₖ)
```

This is fundamental to the physical model - the resonator modes are inherently sinusoidal.

## Problem with Previous Implementation

The previous implementation (now removed) applied different waveforms (sawtooth, square, triangle, pulse) to the **oscillator output** at the synthesis stage. This violated the physical model by changing the basis functions from sinusoidal eigenmodes to other waveforms.

**Issues:**
- Broke physical interpretation of `aₖ` as eigenmode coefficients
- Not consistent with modal resonator theory
- Could degrade sound quality by introducing non-physical artifacts

## Hybrid Solution

The hybrid approach preserves the physical model while still providing waveform selection as a timbral control:

### 1. Resonator Modes Stay Sinusoidal

Audio synthesis **always** uses sinusoidal oscillators:

```c
// audio_synth.c, line ~217
float oscillator_out = osc_sine(phase);
```

This preserves the fundamental physical interpretation.

### 2. Waveform Affects Drive Distribution

Instead of changing the oscillator, waveform selection affects **how energy is distributed across modes** during excitation.

Different waveforms have different harmonic content:
- **Sine**: Fundamental only
- **Saw**: All harmonics, amplitude ∝ 1/n
- **Square**: Odd harmonics only, amplitude ∝ 1/n
- **Triangle**: Odd harmonics only, amplitude ∝ 1/n²
- **Pulse**: Complex pattern depending on duty cycle

### 3. Spectral Shaping Function

The `waveform_harmonic_amplitude()` function returns the relative amplitude for each harmonic:

```c
float waveform_harmonic_amplitude(wave_shape_t waveform, int harmonic_idx) {
    // harmonic_idx: 0 = fundamental, 1 = 2nd harmonic, etc.
    int n = harmonic_idx + 1;

    switch (waveform) {
        case WAVE_SHAPE_SINE:
            return (harmonic_idx == 0) ? 1.0f : 0.0f;
        case WAVE_SHAPE_SAWTOOTH:
            return 1.0f / (float)n;
        case WAVE_SHAPE_SQUARE:
            return (n % 2 == 1) ? (1.0f / (float)n) : 0.0f;
        // ... etc
    }
}
```

### 4. Application Points

Spectral shaping is applied at two points:

**A. During Poke Events** (`modal_node_apply_poke`):
```c
// Get harmonic amplitude for this mode
float harmonic_amplitude = waveform_harmonic_amplitude(node->excitation_waveform, k);

// Apply to effective weight
float effective_weight = base_weight * harmonic_amplitude;
```

**B. During Continuous Excitation** (`modal_node_step`):
```c
// Apply waveform-based spectral shaping to excitation term
float harmonic_amplitude = waveform_harmonic_amplitude(node->excitation_waveform, k);
float strength = node->excitation.strength * mode->params.weight * harmonic_amplitude;
```

## Implementation Details

### Data Structure Changes

Added to `modal_node_t`:
```c
wave_shape_t excitation_waveform;  ///< Waveform for spectral shaping of drive
```

### Function Changes

**modal_node.h:**
- Added `waveform_harmonic_amplitude()` function declaration

**modal_node.c:**
- Implemented `waveform_harmonic_amplitude()` with accurate harmonic profiles
- Modified `modal_node_init()` to initialize `excitation_waveform`
- Modified `modal_node_apply_poke()` to apply spectral shaping
- Modified `modal_node_step()` excitation term to apply spectral shaping

**audio_synth.c:**
- Removed oscillator waveform switching
- Always use `osc_sine()` for physical correctness
- Added documentation explaining the hybrid approach

### Legacy Parameters

The per-mode `shape` parameter in `mode_params_t` is now unused (marked as legacy). The global `excitation_waveform` in `modal_node_t` controls spectral distribution.

## Practical Benefits

1. **Physically Correct**: Preserves modal resonator theory
2. **Timbral Control**: Still provides waveform selection as a sound design tool
3. **Better Quality**: Avoids non-physical artifacts
4. **Conceptually Clear**: Separates resonator physics (always sine) from excitation shaping

## Example

Consider a node with 4 modes at harmonic frequencies (100, 200, 300, 400 Hz):

**Sine Wave Excitation:**
- Mode 0: 100% excitation (fundamental)
- Mode 1-3: 0% excitation (no harmonics)
- Result: Pure tone

**Sawtooth Excitation:**
- Mode 0: 100% excitation (fundamental, 1/1)
- Mode 1: 50% excitation (2nd harmonic, 1/2)
- Mode 2: 33% excitation (3rd harmonic, 1/3)
- Mode 3: 25% excitation (4th harmonic, 1/4)
- Result: Rich, buzzy tone (but still pure sine modes)

**Square Wave Excitation:**
- Mode 0: 100% excitation (fundamental, odd)
- Mode 1: 0% excitation (2nd harmonic, even)
- Mode 2: 33% excitation (3rd harmonic, odd)
- Mode 3: 0% excitation (4th harmonic, even)
- Result: Hollow, reed-like tone

## Future Extensions

- Add ADSR envelope to excitation_waveform parameter
- Allow per-mode spectral shaping curves
- Implement formant-like spectral envelopes
- Add continuous morphing between waveform types

## References

- Modal synthesis theory: "Modal Synthesis" (Julius O. Smith)
- Physical modeling: "Physical Audio Signal Processing" (Julius O. Smith)
- Van der Pol oscillators and modal coupling
