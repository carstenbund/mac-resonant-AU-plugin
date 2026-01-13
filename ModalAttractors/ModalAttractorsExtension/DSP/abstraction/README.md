# DSP Abstraction Layer

This directory implements a control mapping abstraction layer that allows versatile use of the core modal synthesis DSP. It provides a unified interface for both MIDI and CV/Gate control without modifying the DSP core.

## Architecture Overview

The abstraction layer consists of four main components:

### 1. ModalVoiceCore
**Files:** `ModalVoiceCore.h`, `ModalVoiceCore.cpp`

DSP host wrapper that encapsulates `modal_node_t` and `audio_synth_t`. Provides a clean C++ interface for:
- Pitch control (Hz and semitone bend)
- Global and per-mode damping
- Mode weight configuration
- Excitation triggering
- Audio rendering

### 2. InputMapper
**Files:** `InputMapper.h`, `InputMapper.cpp`

Protocol-agnostic input mapper that maintains a unified `IntentState` representing musical control intent. Features:
- MIDI input handling (Note On/Off, Pitch Bend, CC)
- CV/Gate input handling (1V/oct pitch, gate, trigger, energy, damping, brightness)
- Brightness-to-mode-weight mapping
- Edge detection for triggers
- 500 Hz control rate commitment to voice

### 3. MidiAdapter
**File:** `MidiAdapter.h`

Thin pass-through adapter for MIDI events. Forwards MIDI messages to `InputMapper`.

### 4. CvAdapter
**File:** `CvAdapter.h`

Thin pass-through adapter for CV/Gate signals. Forwards CV voltages to `InputMapper`.

## Usage Example (AU Integration)

```cpp
#include "abstraction/ModalVoiceCore.h"
#include "abstraction/InputMapper.h"
#include "abstraction/MidiAdapter.h"

class MySynthEngine {
public:
    void initialize(float sample_rate) {
        voice_.initialize(sample_rate);
    }

    void processNoteOn(uint8_t note, uint8_t velocity) {
        midiAdapter_.handleNoteOn(note, velocity);
    }

    void processNoteOff(uint8_t note) {
        midiAdapter_.handleNoteOff(note);
    }

    void updateControlRate() {
        // Called at 500 Hz
        inputMapper_.commitToVoice(voice_);
        voice_.processControlTick();
    }

    void renderAudio(float* outL, float* outR, uint32_t frames) {
        voice_.renderAudio(outL, outR, frames);
    }

private:
    ModalVoiceCore voice_{0};
    InputMapper inputMapper_;
    MidiAdapter midiAdapter_{inputMapper_};
};
```

## Usage Example (ESP32 CV Integration)

```cpp
#include "abstraction/ModalVoiceCore.h"
#include "abstraction/InputMapper.h"
#include "abstraction/CvAdapter.h"

// ADC sampling loop (fast)
void cv_sample_loop() {
    float pitch_v = read_adc_pitch();
    float energy_v = read_adc_energy();
    float gate_v = read_adc_gate();

    cvAdapter_.setPitchVolts(pitch_v);
    cvAdapter_.setEnergyVolts(energy_v);
    cvAdapter_.setGateVolts(gate_v);
}

// Control tick (500 Hz)
void control_tick_2ms() {
    inputMapper_.commitToVoice(voice_);
    voice_.processControlTick();
}

// Audio render callback
void audio_callback(float* outL, float* outR, uint32_t frames) {
    voice_.renderAudio(outL, outR, frames);
}
```

## MIDI CC Mapping

The `InputMapper` responds to the following MIDI CC messages:

- **CC 1** (Modulation Wheel) → Brightness
- **CC 16** (General Purpose 1) → Poke Duration [1..20 ms]
- **CC 71** (Resonance) → Global Damping (inverted)
- **CC 74** (Brightness/Cutoff) → Brightness

## CV/Gate Mapping

The `CvAdapter` expects the following CV signal ranges:

- **Pitch:** 1V/octave (0V = C1 @ 32.7 Hz)
- **Energy:** 0-5V → Excitation strength [0..1]
- **Gate:** >2.5V = high, triggers on rising edge
- **Trigger:** >2.5V = high, explicit trigger pulse
- **Damping:** 0-5V → Global damping [0..1]
- **Brightness:** 0-5V → Mode weight distribution [0..1]

## Design Principles

1. **No DSP Core Modification:** The abstraction layer sits entirely outside the core DSP (`modal_node.h/c`, `audio_synth.h/c`)
2. **Unified Control Semantics:** MIDI and CV share the same mapping logic through `IntentState`
3. **Real-time Safety:** No dynamic allocation, fixed buffer sizes
4. **Clean Separation:** Thin adapters keep protocol-specific code isolated
5. **Cross-platform:** Same abstraction works on macOS AU, ESP32 hardware, and future platforms

## Control Rate

The system operates at a 500 Hz control rate (2ms timestep), matching the existing `CONTROL_RATE_HZ` in the DSP core. This rate is sufficient for:
- Smooth parameter updates
- Envelope processing
- Gate/trigger edge detection
- Modal dynamics integration

## Integration Notes

- Call `commitToVoice()` at 500 Hz before `processControlTick()`
- Audio rendering (`renderAudio()`) can be called at any rate/buffer size
- Trigger events are one-shot: `trig_edge` is automatically cleared after commit
- Brightness parameter controls mode weight distribution (0 = fundamental only, 1 = bright harmonics)

## Future Extensions

- Multi-voice polyphony support
- Advanced modulation routing (LFOs, envelopes)
- Preset management
- Network coupling between voices
- MIDI MPE support for per-note expression
