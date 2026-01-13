/**
 * @file ModalVoiceCore.cpp
 * @brief Implementation of DSP host wrapper for modal voice control
 */

#include "ModalVoiceCore.h"
#include <cmath>
#include <algorithm>

ModalVoiceCore::ModalVoiceCore(uint8_t voice_id)
    : voice_id_(voice_id)
{
}

void ModalVoiceCore::initialize(float sample_rate) {
    sample_rate_ = sample_rate;

    // Initialize modal node as a resonator
    modal_node_init(&node_, voice_id_, PERSONALITY_RESONATOR);

    // Set default mode configuration (harmonic series)
    const float base_omega = freq_to_omega(pitch_hz_);
    for (uint8_t i = 0; i < MAX_MODES; ++i) {
        const float harmonic = static_cast<float>(i + 1);
        const float omega = base_omega * harmonic;
        const float gamma = 2.0f + (harmonic * 0.5f); // Increasing damping for higher modes
        const float weight = 1.0f / harmonic;          // Decreasing weight for higher modes

        modal_node_set_mode(&node_, i, omega, gamma, weight);
    }

    // Initialize audio synthesis
    audio_synth_init(&synth_, &node_, sample_rate_);

    // Start the node
    modal_node_start(&node_);

    initialized_ = true;
}

void ModalVoiceCore::setPitchHz(float hz) {
    pitch_hz_ = std::clamp(hz, 20.0f, 20000.0f);
    updateFrequencies();
}

void ModalVoiceCore::setPitchBend(float semis) {
    pitch_bend_semis_ = std::clamp(semis, -24.0f, 24.0f);
    updateFrequencies();
}

void ModalVoiceCore::setGlobalDamping(float damping_0to1) {
    global_damping_ = std::clamp(damping_0to1, 0.0f, 1.0f);

    // Map [0,1] to reasonable damping range [0, 10]
    node_.global_damping = global_damping_ * 10.0f;
}

void ModalVoiceCore::setModeDamping(uint8_t mode, float gamma) {
    if (mode >= MAX_MODES) return;

    node_.modes[mode].params.gamma = std::max(0.0f, gamma);
}

void ModalVoiceCore::setModeWeight(uint8_t mode, float weight) {
    if (mode >= MAX_MODES) return;

    node_.modes[mode].params.weight = std::clamp(weight, 0.0f, 1.0f);
}

void ModalVoiceCore::setExcitationParams(const ExcitationParams& params) {
    excitation_ = params;

    // Clamp values
    excitation_.strength = std::clamp(excitation_.strength, 0.0f, 1.0f);
    excitation_.duration_ms = std::clamp(excitation_.duration_ms, 1.0f, 20.0f);

    // Apply mode weights to the node's excitation envelope
    node_.excitation.duration_ms = excitation_.duration_ms;
}

void ModalVoiceCore::trigger() {
    if (!initialized_) return;

    // Build poke event from current excitation parameters
    poke_event_t poke;
    poke.source_node_id = voice_id_;
    poke.strength = excitation_.strength;
    poke.duration_ms = excitation_.duration_ms;
    poke.phase_hint = excitation_.phase_hint;

    // Copy mode weights
    for (uint8_t i = 0; i < MAX_MODES; ++i) {
        poke.mode_weights[i] = excitation_.mode_weights[i];
    }

    // Apply the poke to the modal node
    modal_node_apply_poke(&node_, &poke);
}

void ModalVoiceCore::processControlTick() {
    if (!initialized_) return;

    // Step the modal dynamics (500 Hz control rate)
    modal_node_step(&node_);
}

void ModalVoiceCore::renderAudio(float* outL, float* outR, uint32_t frames) {
    if (!initialized_) {
        // Output silence if not initialized
        for (uint32_t i = 0; i < frames; ++i) {
            outL[i] = 0.0f;
            outR[i] = 0.0f;
        }
        return;
    }

    // Render audio from modal state
    audio_synth_render(&synth_, outL, outR, frames);
}

void ModalVoiceCore::updateFrequencies() {
    if (!initialized_) return;

    // Calculate actual pitch with bend applied
    const float bend_ratio = std::pow(2.0f, pitch_bend_semis_ / 12.0f);
    const float actual_hz = pitch_hz_ * bend_ratio;

    // Update carrier frequency
    node_.carrier_freq_hz = actual_hz;

    // Update all mode frequencies (harmonic series)
    const float base_omega = freq_to_omega(actual_hz);
    for (uint8_t i = 0; i < MAX_MODES; ++i) {
        const float harmonic = static_cast<float>(i + 1);
        node_.modes[i].params.omega = base_omega * harmonic;
    }
}
