/**
 * @file InputMapper.cpp
 * @brief Implementation of protocol-agnostic input mapper
 */

#include "InputMapper.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// MIDI Input Methods
// ============================================================================

void InputMapper::onMidiNoteOn(uint8_t note, uint8_t velocity) {
    current_note_ = std::clamp(note, uint8_t(0), uint8_t(127));
    note_active_ = true;

    // Convert MIDI note to frequency
    intent_.pitch_hz = midi_to_freq(current_note_);

    // Convert velocity to energy [0..1]
    intent_.energy = static_cast<float>(velocity) / 127.0f;

    // Set gate on
    intent_.gate = true;

    // Trigger edge on note on
    intent_.trig_edge = true;
}

void InputMapper::onMidiNoteOff(uint8_t note) {
    if (note == current_note_ && note_active_) {
        note_active_ = false;
        intent_.gate = false;
        intent_.energy = 0.0f;
    }
}

void InputMapper::onMidiPitchBend(float semis) {
    intent_.pitch_bend_semis = std::clamp(semis, -24.0f, 24.0f);
}

void InputMapper::onMidiCC(uint8_t cc, uint8_t value) {
    const float normalized = static_cast<float>(value) / 127.0f;

    switch (cc) {
        case 1:  // Modulation wheel -> brightness
            intent_.brightness = normalized;
            updateModeWeights();
            break;

        case 7:  // Volume -> not used (handled by master gain elsewhere)
            break;

        case 10: // Pan -> not used (mono synthesis)
            break;

        case 71: // Resonance/Filter Resonance -> damping (inverted)
            intent_.global_damping = 1.0f - normalized;
            break;

        case 74: // Brightness/Filter Cutoff -> brightness
            intent_.brightness = normalized;
            updateModeWeights();
            break;

        case 16: // General Purpose 1 -> poke duration
            intent_.duration_ms = 1.0f + (normalized * 19.0f); // [1..20] ms
            break;

        default:
            // Ignore other CCs
            break;
    }
}

// ============================================================================
// CV Input Methods
// ============================================================================

void InputMapper::setCvPitchVolts(float volts) {
    // 1V/octave standard: 0V = C1 (32.7 Hz), each volt doubles frequency
    // MIDI note 0 = C-1 = 8.176 Hz, so 0V CV = MIDI 36 (C1)
    const float base_note = 36.0f;  // C1 as reference (0V)
    const float note = base_note + (volts * 12.0f); // 1V/oct = 12 semitones/volt

    // Convert to frequency
    intent_.pitch_hz = 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

void InputMapper::setCvEnergyVolts(float volts) {
    // 0..5V -> 0..1
    intent_.energy = std::clamp(volts / 5.0f, 0.0f, 1.0f);
}

void InputMapper::setCvGateVolts(float volts) {
    // Gate threshold at 2.5V (Eurorack standard)
    const bool gate_state = (volts > 2.5f);
    intent_.gate = gate_state;

    // Detect rising edge
    if (gate_state && !last_gate_) {
        intent_.trig_edge = true;
    }

    last_gate_ = gate_state;
}

void InputMapper::setCvTrigVolts(float volts) {
    // Trigger threshold at 2.5V
    const bool trig_state = (volts > 2.5f);

    // Detect rising edge
    if (trig_state && !last_trig_) {
        intent_.trig_edge = true;
    }

    last_trig_ = trig_state;
}

void InputMapper::setCvDampVolts(float volts) {
    // 0..5V -> 0..1
    intent_.global_damping = std::clamp(volts / 5.0f, 0.0f, 1.0f);
}

void InputMapper::setCvBrightVolts(float volts) {
    // 0..5V -> 0..1
    intent_.brightness = std::clamp(volts / 5.0f, 0.0f, 1.0f);
    updateModeWeights();
}

// ============================================================================
// Control Output
// ============================================================================

void InputMapper::commitToVoice(ModalVoiceCore& voice) {
    // Update voice parameters
    voice.setPitchHz(intent_.pitch_hz);
    voice.setPitchBend(intent_.pitch_bend_semis);
    voice.setGlobalDamping(intent_.global_damping);

    // Update excitation parameters
    ExcitationParams excitation;
    excitation.strength = intent_.energy;
    excitation.duration_ms = intent_.duration_ms;
    excitation.phase_hint = intent_.phase_hint;

    // Copy mode weights
    for (uint8_t i = 0; i < MAX_MODES; ++i) {
        excitation.mode_weights[i] = mode_weights_[i];
    }

    voice.setExcitationParams(excitation);

    // Trigger if edge detected
    if (intent_.trig_edge) {
        voice.trigger();
        intent_.trig_edge = false; // Clear edge flag after trigger
    }
}

// ============================================================================
// Private Methods
// ============================================================================

void InputMapper::updateModeWeights() {
    // Map brightness to mode weight distribution
    // brightness = 0.0 -> only fundamental (mode 0)
    // brightness = 0.5 -> balanced harmonic series
    // brightness = 1.0 -> emphasize higher modes

    const float b = std::clamp(intent_.brightness, 0.0f, 1.0f);

    for (uint8_t i = 0; i < MAX_MODES; ++i) {
        const float mode_idx = static_cast<float>(i);

        // Exponential weighting based on brightness
        // Lower brightness = stronger rolloff for higher modes
        const float rolloff = std::exp(-mode_idx * (1.0f - b) * 0.5f);
        mode_weights_[i] = rolloff;
    }

    // Normalize weights to sum to MAX_MODES (preserve total energy)
    float sum = 0.0f;
    for (uint8_t i = 0; i < MAX_MODES; ++i) {
        sum += mode_weights_[i];
    }

    if (sum > 0.0f) {
        const float scale = static_cast<float>(MAX_MODES) / sum;
        for (uint8_t i = 0; i < MAX_MODES; ++i) {
            mode_weights_[i] *= scale;
        }
    }
}

void InputMapper::edgeDetectTrigger() {
    // Edge detection is now handled inline in setCvGateVolts and setCvTrigVolts
    // This method is kept for potential future use or additional edge detection logic
}
