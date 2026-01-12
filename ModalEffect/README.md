# ModalEffect - Resonant Body Effect Plugin

## Overview

ModalEffect is an audio effect plugin based on modal synthesis that transforms input audio through a physically-modeled resonant body. Unlike the ModalAttractors synthesizer, ModalEffect is an **audio effect** that processes incoming audio signals rather than generating sound from MIDI.

## Architecture

### Core DSP Components

ModalEffect implements a complete signal processing chain with the following components:

#### 1. **EnergyExtractor** (`EnergyExtractor.h/cpp`)
- **Purpose**: Extracts the energy envelope from the input signal
- **Method**: RMS (Root Mean Square) analysis with attack/release envelope follower
- **Parameters**:
  - Attack time: 5ms (fast response to transients)
  - Release time: 100ms (smooth decay tracking)
  - RMS window: 10ms (energy averaging)
- **Implementation**: Circular buffer for RMS calculation, exponential envelope smoothing

#### 2. **SpectralAnalyzer** (`SpectralAnalyzer.h/cpp`)
- **Purpose**: Splits the input signal into three frequency bands
- **Method**: 3-band biquad filter bank (Butterworth design)
- **Bands**:
  - **Low**: DC to 300 Hz (bass frequencies)
  - **Mid**: 300 Hz to 3000 Hz (midrange)
  - **High**: 3000 Hz to Nyquist (treble)
- **Implementation**: 2nd-order Butterworth lowpass, bandpass, and highpass filters

#### 3. **PitchDetector** (`PitchDetector.h/cpp`)
- **Purpose**: Detects the fundamental frequency of the input signal
- **Method**: Autocorrelation-based pitch detection
- **Parameters**:
  - Frequency range: 60 Hz - 2000 Hz
  - Analysis window: 50ms
  - Smoothing: 100ms exponential smoothing
- **Implementation**: Circular buffer, autocorrelation peak finding, confidence threshold

#### 4. **ResonantBodyProcessor** (`ResonantBodyProcessor.h/cpp`)
- **Purpose**: Main effect processor integrating all components
- **Architecture**:
  - Uses 3 modal resonators (one per frequency band)
  - Each resonator has 4 modes with slightly inharmonic relationships
  - Modes: 1.0x, 2.3x, 3.7x, 5.2x fundamental (harmonic + slight inharmonicity)
- **Processing Flow**:
  1. Extract energy envelope from input
  2. Split input into 3 frequency bands
  3. Update pitch detector
  4. Excite resonators with band-filtered signals scaled by energy
  5. Render modal resonator audio
  6. Mix dry and wet signals

## User Parameters

ModalEffect provides 5 intuitive effect-style parameters:

### 1. **Body Size** (0-1)
- **Description**: Scales the resonator frequencies
- **Effect**:
  - 0 (Small): High-pitched, bright resonance
  - 1 (Large): Low-pitched, deep resonance
- **Frequency multiplier range**: 0.5× to 2.0×

### 2. **Material** (0-1)
- **Description**: Controls damping (resonance decay time)
- **Effect**:
  - 0 (Soft): High damping, short decay (muted, wood-like)
  - 1 (Hard): Low damping, long ring (metallic, bell-like)
- **Damping range**: 0.5 to 50.0 (higher = faster decay)

### 3. **Excite** (0-1)
- **Description**: How much the input drives the resonator
- **Effect**:
  - 0: Minimal excitation
  - 1: Maximum excitation (strong resonance)

### 4. **Morph** (0-1)
- **Description**: Pitch tracking amount for morphing resonance
- **Effect**:
  - 0: Fixed resonance frequencies based on Body Size
  - 1: Resonance follows the detected pitch of the input
- **Use case**: Create pitch-tracking resonances that adapt to the input

### 5. **Mix** (0-1)
- **Description**: Dry/wet balance
- **Effect**:
  - 0: 100% dry (original signal)
  - 1: 100% wet (resonated signal only)

## Signal Flow

```
Input Audio
    │
    ├──→ EnergyExtractor ──→ Energy Envelope
    │
    ├──→ SpectralAnalyzer ──→ [Low, Mid, High] Bands
    │
    └──→ PitchDetector ──→ Fundamental Frequency
             │
             ▼
    [Control Rate: ~240 Hz]
    - Update resonator frequencies based on Morph
    - Update modal dynamics
             │
             ▼
    Band Excitation × Energy × Excite
             │
             ▼
    3× Modal Resonators (4 modes each)
    - Low band → Resonator 0 (150 Hz base)
    - Mid band → Resonator 1 (600 Hz base)
    - High band → Resonator 2 (2400 Hz base)
             │
             ▼
    Synthesis (sine wave carriers + modal amplitudes)
             │
             ▼
    Mix (dry + wet)
             │
             ▼
    Output Audio
```

## Technical Details

### Modal Synthesis
Each resonator uses the existing `modal_node.c` implementation from ModalAttractors:
- 4 complex modes per resonator
- Dynamics: ȧ_k = (-γ_k + iω_k)a_k + u_k(t)
- Wave shape: Sine (for clean resonance)
- Personality: Resonator (decays to silence, percussive response)

### Control Rate
- Audio rate: Variable (typically 48 kHz)
- Control rate: ~240 Hz (updates every ~200 samples at 48 kHz)
- Pitch detection: Analyzed every buffer (~512 samples typical)

### Real-time Safety
- Pre-allocated buffers in init()
- No dynamic allocation in process loop
- Lock-free parameter updates

## File Structure

```
ModalEffect/
├── ModalEffectExtension/
│   ├── DSP/
│   │   ├── EnergyExtractor.h / .cpp           # RMS energy extraction
│   │   ├── SpectralAnalyzer.h / .cpp          # 3-band filter bank
│   │   ├── PitchDetector.h / .cpp             # Autocorrelation pitch detection
│   │   ├── ResonantBodyProcessor.h / .cpp     # Main effect processor
│   │   ├── modal_node.h / .c                  # Modal resonator (from ModalAttractors)
│   │   ├── audio_synth.h / .c                 # Audio synthesis (from ModalAttractors)
│   │   └── ... (other DSP files)
│   ├── Parameters/
│   │   ├── Parameters.swift                   # Parameter definitions
│   │   └── ModalEffectExtensionParameterAddresses.h
│   └── ...
└── README.md (this file)
```

## Differences from ModalAttractors

| Feature | ModalAttractors | ModalEffect |
|---------|-----------------|-------------|
| **Type** | MIDI Synthesizer | Audio Effect |
| **Input** | MIDI notes | Audio signal |
| **Resonators** | 5 nodes (network topology) | 3 resonators (frequency bands) |
| **Excitation** | MIDI note-on events | Continuous audio input |
| **Parameters** | 47 parameters (complex) | 5 parameters (simple) |
| **Coupling** | Network coupling between nodes | No coupling (independent bands) |
| **Use case** | Generative synthesis | Audio processing/coloration |

## Future Enhancements

Possible improvements:
1. **Band crossover controls**: User-adjustable crossover frequencies
2. **Per-band mix**: Independent wet/dry for each band
3. **Resonator coupling**: Add coupling between band resonators
4. **Alternative synthesis**: Try different wave shapes or FM
5. **Modulation**: LFO or envelope followers for parameters
6. **Presets**: Character presets for different material types

## Building

The ModalEffect plugin uses the same build system as ModalAttractors:
- Xcode project: `ModalEffect.xcodeproj`
- Targets: macOS app + Audio Unit extension
- Minimum macOS: 11.0

## Credits

Based on the ModalAttractors modal synthesis engine by Carsten Bund.
ModalEffect adaptation implements the resonant body effect concept with new DSP components for audio processing.
