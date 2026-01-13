# Control Mapping Architecture (AU + ESP32)

This proposal keeps the DSP core untouched and adds a control/mapping layer that can accept MIDI or CV/Gate inputs. It uses the existing `poke_event_t` definition and the current `audio_synth_render()` signature as the DSP seam, so the AU and ESP32 share the same excitation model and rendering API.

## 1) DSP Core Contract (existing seam)

The core DSP contract already exposes the two operations the wrapper needs:

- **Excite**: `modal_node_apply_poke(modal_node_t*, const poke_event_t*)` with `poke_event_t` carrying strength, phase hint, and per-mode weights.
- **Render**: `audio_synth_render(audio_synth_t*, float* outL, float* outR, uint32_t num_frames)` for pull-based audio.

That means the control layer can remain 100% outside the DSP core.

## 2) Proposed Header Layout

### `ModalVoiceCore.h` (DSP host wrapper)

```cpp
#pragma once

#include "modal_node.h"
#include "audio_synth.h"

struct ExcitationParams {
    float strength = 0.0f;   // 0..1
    float duration_ms = 8.0f; // planned (see note below)
    float phase_hint = -1.0f; // -1 = random
    float mode_weights[MAX_MODES] = {1.0f, 1.0f, 1.0f, 1.0f};
};

class ModalVoiceCore {
public:
    explicit ModalVoiceCore(uint8_t voice_id);

    void initialize(float sample_rate);
    void setPitchHz(float hz);
    void setPitchBend(float semis);
    void setGlobalDamping(float damping_0to1);
    void setModeDamping(uint8_t mode, float gamma);
    void setModeWeight(uint8_t mode, float weight);
    void setExcitationParams(const ExcitationParams& params);
    void trigger();

    void processControlTick(); // 500 Hz
    void renderAudio(float* outL, float* outR, uint32_t frames);

private:
    void updateFrequencies();

    uint8_t voice_id_ = 0;
    float sample_rate_ = 48000.0f;
    float pitch_hz_ = 440.0f;
    float pitch_bend_semis_ = 0.0f;
    float global_damping_ = 0.0f;
    ExcitationParams excitation_{};

    modal_node_t node_{};
    audio_synth_t synth_{};
};
```

> **Note on `duration_ms`**: the AU already exposes a poke duration parameter, but `poke_event_t` currently has no field for it. This doc assumes you will add `duration_ms` to `poke_event_t` in the DSP core once you are ready (both macOS port + ESP32).

### `InputMapper.h` (protocol-agnostic)

```cpp
#pragma once

#include <cstdint>
#include "ModalVoiceCore.h"

struct IntentState {
    float pitch_hz = 440.0f;
    float energy = 0.0f;        // 0..1
    float duration_ms = 8.0f;   // poke duration
    float brightness = 0.5f;    // 0..1
    float global_damping = 0.0f; // 0..1
    float pitch_bend_semis = 0.0f;
    bool gate = false;
    bool trig_edge = false;
    float phase_hint = -1.0f;
};

class InputMapper {
public:
    void onMidiNoteOn(uint8_t note, uint8_t velocity);
    void onMidiNoteOff(uint8_t note);
    void onMidiPitchBend(float semis);
    void onMidiCC(uint8_t cc, uint8_t value);

    void setCvPitchVolts(float volts);
    void setCvEnergyVolts(float volts);
    void setCvGateVolts(float volts);
    void setCvTrigVolts(float volts);
    void setCvDampVolts(float volts);
    void setCvBrightVolts(float volts);

    // Called at 500 Hz
    void commitToVoice(ModalVoiceCore& voice);

private:
    void updateModeWeights();
    void edgeDetectTrigger();

    IntentState intent_{};
    bool last_gate_ = false;
};
```

### `MidiAdapter.h` + `CvAdapter.h` (thin I/O translators)

```cpp
#pragma once
#include "InputMapper.h"

class MidiAdapter {
public:
    explicit MidiAdapter(InputMapper& mapper) : mapper_(mapper) {}

    void handleNoteOn(uint8_t note, uint8_t velocity) { mapper_.onMidiNoteOn(note, velocity); }
    void handleNoteOff(uint8_t note) { mapper_.onMidiNoteOff(note); }
    void handlePitchBend(float semis) { mapper_.onMidiPitchBend(semis); }
    void handleCC(uint8_t cc, uint8_t value) { mapper_.onMidiCC(cc, value); }

private:
    InputMapper& mapper_;
};

class CvAdapter {
public:
    explicit CvAdapter(InputMapper& mapper) : mapper_(mapper) {}

    void setPitchVolts(float v) { mapper_.setCvPitchVolts(v); }
    void setEnergyVolts(float v) { mapper_.setCvEnergyVolts(v); }
    void setGateVolts(float v) { mapper_.setCvGateVolts(v); }
    void setTrigVolts(float v) { mapper_.setCvTrigVolts(v); }
    void setDampVolts(float v) { mapper_.setCvDampVolts(v); }
    void setBrightVolts(float v) { mapper_.setCvBrightVolts(v); }

private:
    InputMapper& mapper_;
};
```

## 3) AU Integration Example (SynthEngine)

In the AU `SynthEngine::processEvent()` path, replace direct node manager calls with mapping into the shared `InputMapper`, then commit at control rate. This matches the existing event dispatch structure that already collects events by sample offset and updates at the control rate step.

```cpp
void SynthEngine::processEvent(const SynthEvent& event) {
    switch (event.type) {
        case EventType::NoteOn:
            midiAdapter_.handleNoteOn(event.noteOn.note, event.noteOn.velocity);
            break;
        case EventType::NoteOff:
            midiAdapter_.handleNoteOff(event.noteOff.note);
            break;
        case EventType::PitchBend:
            midiAdapter_.handlePitchBend(event.pitchBend.value);
            break;
        case EventType::CC:
            midiAdapter_.handleCC(event.cc.controller, event.cc.value);
            break;
        case EventType::Parameter:
            setParameter(event.parameter.paramId, event.parameter.value);
            break;
    }
}

void SynthEngine::updateControlRate() {
    inputMapper_.commitToVoice(modalVoiceCore_);
    modalVoiceCore_.processControlTick();
}
```

## 4) ESP32 Integration Example (CV ISR / ADC loop)

The ESP32 front-end becomes a simple producer of CV values. In an ADC loop or ISR, feed the same `InputMapper` that the AU uses; at 500 Hz call `commitToVoice()` and `processControlTick()`.

```cpp
// ISR or ADC task (fast)
void cv_sample_loop() {
    float pitch_v = read_adc_pitch();   // 1V/oct
    float energy_v = read_adc_energy(); // 0..5V
    float gate_v = read_adc_gate();
    float trig_v = read_adc_trig();
    float damp_v = read_adc_damp();
    float bright_v = read_adc_bright();

    cvAdapter_.setPitchVolts(pitch_v);
    cvAdapter_.setEnergyVolts(energy_v);
    cvAdapter_.setGateVolts(gate_v);
    cvAdapter_.setTrigVolts(trig_v);
    cvAdapter_.setDampVolts(damp_v);
    cvAdapter_.setBrightVolts(bright_v);
}

// 500 Hz control loop
void control_tick_2ms() {
    inputMapper_.commitToVoice(modalVoiceCore_);
    modalVoiceCore_.processControlTick();
}
```

## 5) Why this maps cleanly to your existing DSP

- `ModalVoiceCore` is the only place that touches `modal_node_*` and `audio_synth_*`.
- `InputMapper` contains *all* input logic (velocity scaling, CV normalization, edge detection).
- AU/ESP32 share the same mapping and control semantics.

This keeps the DSP core unchanged while unifying MIDI and CV behavior across all platforms.
