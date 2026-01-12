/**
 * @file ModalVoice.cpp
 * @brief C++ wrapper implementation for modal_node C core
 */

#include "ModalVoice.h"
#include <cmath>
#include <cstring>

ModalVoice::ModalVoice(uint8_t voice_id)
    : voice_id_(voice_id)
    , state_(State::Inactive)
    , midi_note_(60)
    , velocity_(0.0f)
    , pitch_bend_(0.0f)
    , age_(0)
    , samples_since_update_(0)
    , samples_per_update_(0)
    , sample_rate_(48000.0f)
{
    // Initialize node with resonator personality by default
    modal_node_init(&node_, voice_id, PERSONALITY_RESONATOR);
}

ModalVoice::~ModalVoice() {
    // Clean up (nothing to do for POD types)
}

void ModalVoice::initialize(float sample_rate) {
    sample_rate_ = sample_rate;

    // Calculate samples per control update
    // Control rate is 500 Hz (CONTROL_RATE_HZ)
    samples_per_update_ = static_cast<uint32_t>(sample_rate / CONTROL_RATE_HZ);

    // Initialize audio synth
    audio_synth_init(&synth_, &node_, sample_rate);

    // Set default mode configuration (4 harmonically related modes)
    float base_freq = midi_to_freq(midi_note_);
    setMode(0, base_freq * 1.0f, 0.5f, 1.0f);      // Fundamental
    setMode(1, base_freq * 1.01f, 0.6f, 0.7f);     // Slight detune
    setMode(2, base_freq * 2.0f, 0.8f, 0.5f);      // Second harmonic
    setMode(3, base_freq * 3.0f, 1.0f, 0.3f);      // Third harmonic

    // Start node
    modal_node_start(&node_);
}

void ModalVoice::noteOn(uint8_t midi_note, float velocity) {
    midi_note_ = midi_note;
    velocity_ = velocity;
    state_ = State::Attack;
    age_ = 0;

    // Update frequencies based on new note
    updateFrequencies();

    // Reset phase accumulators to prevent clicks/discontinuities
    audio_synth_reset_phase(&synth_);

    // Apply poke excitation
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

void ModalVoice::noteOff() {
    if (state_ == State::Inactive) return;

    state_ = State::Release;
}

void ModalVoice::setPitchBend(float bend_amount, float bend_range) {
    pitch_bend_ = bend_amount;
    updateFrequencies();
}

void ModalVoice::updateModal() {
    if (state_ == State::Inactive) return;

    // Step modal dynamics
    modal_node_step(&node_);

    // Update state machine
    updateState();

    // Increment age
    age_++;
}

void ModalVoice::renderAudio(float* outL, float* outR, uint32_t num_frames) {
    if (state_ == State::Inactive) {
        // Silent voice - write zeros
        memset(outL, 0, num_frames * sizeof(float));
        memset(outR, 0, num_frames * sizeof(float));
        return;
    }

    // Update modal state at control rate (500 Hz)
    samples_since_update_ += num_frames;
    while (samples_since_update_ >= samples_per_update_) {
        updateModal();
        samples_since_update_ -= samples_per_update_;
    }

    // Render audio from modal state
    audio_synth_render(&synth_, outL, outR, num_frames);
}

void ModalVoice::applyCoupling(const float coupling_inputs[MAX_MODES]) {
    // Apply coupling inputs to node
    // This modulates the mode amplitudes based on neighbor voices
    for (int k = 0; k < MAX_MODES; k++) {
        if (!node_.modes[k].params.active) continue;

        // Add coupling as excitation
        float coupling_strength = node_.coupling_strength * coupling_inputs[k];
        node_.modes[k].a += coupling_strength * CONTROL_DT;
    }
}

void ModalVoice::applyCouplingMode0(modal_complex_t coupling0) {
    // Apply complex diffusive coupling to mode 0 only
    // This preserves phase information for physically-realistic ensemble coupling
    if (!node_.modes[0].params.active) return;

    // Direct application - coupling strength already applied in TopologyEngine
    // node_.coupling_strength is kept at 1.0 for predictable behavior
    node_.modes[0].a += coupling0 * CONTROL_DT;
}

float ModalVoice::getAmplitude() const {
    return modal_node_get_amplitude(&node_);
}

float ModalVoice::getBaseFrequency() const {
    // Calculate base frequency with pitch bend
    float base_freq = midi_to_freq(midi_note_);
    float bend_factor = powf(2.0f, pitch_bend_ * 2.0f / 12.0f);
    return base_freq * bend_factor;
}

void ModalVoice::setMode(uint8_t mode_idx, float freq_hz, float damping, float weight) {
    if (mode_idx >= MAX_MODES) return;

    float omega = freq_to_omega(freq_hz);
    modal_node_set_mode(&node_, mode_idx, omega, damping, weight);
}

void ModalVoice::setPersonality(node_personality_t personality) {
    node_.personality = personality;
}

void ModalVoice::setGlobalDamping(float damping) {
    node_.global_damping = damping;
}

void ModalVoice::reset() {
    modal_node_reset(&node_);
    state_ = State::Inactive;
    age_ = 0;
    samples_since_update_ = 0;
}

void ModalVoice::updateFrequencies() {
    // Calculate base frequency with pitch bend
    float base_freq = midi_to_freq(midi_note_);

    // Apply pitch bend (±2 semitones by default)
    float bend_factor = powf(2.0f, pitch_bend_ * 2.0f / 12.0f);
    base_freq *= bend_factor;

    // Update all mode frequencies proportionally
    // Mode 0: fundamental
    setMode(0, base_freq * 1.0f, node_.modes[0].params.gamma, node_.modes[0].params.weight);

    // Mode 1: slight detune
    setMode(1, base_freq * 1.01f, node_.modes[1].params.gamma, node_.modes[1].params.weight);

    // Mode 2: second harmonic
    setMode(2, base_freq * 2.0f, node_.modes[2].params.gamma, node_.modes[2].params.weight);

    // Mode 3: third harmonic
    setMode(3, base_freq * 3.0f, node_.modes[3].params.gamma, node_.modes[3].params.weight);
}

void ModalVoice::updateState() {
    // Simple state machine
    switch (state_) {
        case State::Inactive:
            // Nothing to do
            break;

        case State::Attack:
            // Transition to sustain if self-oscillator, else stay in attack
            if (node_.personality == PERSONALITY_SELF_OSCILLATOR) {
                state_ = State::Sustain;
            }
            // Resonator stays in attack until release
            break;

        case State::Sustain:
            // Continue sustaining
            break;

        case State::Release:
            // Check if voice is quiet enough to deactivate
            float amp = getAmplitude();
            if (amp < 0.001f) {
                state_ = State::Inactive;
                reset();
            }
            break;
    }
}
