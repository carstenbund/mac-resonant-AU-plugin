//
//  ResonantBodyProcessor.cpp
//  ModalAttractors
//
//  Resonant body effect processor implementation.
//
//  Created by Claude on 2026-01-11.
//

#include "ResonantBodyProcessor.h"
#include <cmath>
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ResonantBodyProcessor::ResonantBodyProcessor()
    : sample_rate_(48000.0f)
    , body_size_(0.5f)
    , material_(0.5f)
    , excitation_(0.5f)
    , morph_(0.0f)
    , mix_(0.5f)
    , base_frequency_(220.0f)  // A3
    , frequency_scale_(1.0f)
    , samples_until_control_update_(0)
    , control_period_samples_(0)
{
    // Initialize modal node as a resonator (not self-oscillator)
    modal_node_init(&modal_node_, 0, PERSONALITY_RESONATOR);

    // Set default mode frequency multipliers (harmonic series)
    // These create a natural, musical resonance
    mode_freq_multipliers_[0] = 1.0f;   // Fundamental
    mode_freq_multipliers_[1] = 2.0f;   // 1st harmonic (octave)
    mode_freq_multipliers_[2] = 3.0f;   // 2nd harmonic (perfect fifth above octave)
    mode_freq_multipliers_[3] = 5.0f;   // 3rd harmonic (two octaves + major third)

    // Could also use inharmonic ratios for metallic/bell-like sounds:
    // mode_freq_multipliers_[0] = 1.0f;
    // mode_freq_multipliers_[1] = 2.76f;
    // mode_freq_multipliers_[2] = 5.40f;
    // mode_freq_multipliers_[3] = 8.93f;
}

ResonantBodyProcessor::~ResonantBodyProcessor() {
    // Cleanup handled by destructors
}

void ResonantBodyProcessor::initialize(float sample_rate) {
    sample_rate_ = sample_rate;

    // Initialize audio synthesis
    audio_synth_init(&audio_synth_, &modal_node_, sample_rate);

    // Initialize analysis components
    energy_extractor_.initialize(sample_rate);
    spectral_analyzer_.initialize(sample_rate);
    pitch_detector_.initialize(sample_rate);

    // Set analysis parameters
    energy_extractor_.setAttackTime(5.0f);    // Fast attack for transients
    energy_extractor_.setReleaseTime(100.0f); // Moderate release for sustain
    spectral_analyzer_.setSmoothingTime(20.0f);
    pitch_detector_.setWindowSize(40.0f);
    pitch_detector_.setMinPitch(60.0f);       // B1
    pitch_detector_.setMaxPitch(2000.0f);     // B6

    // Calculate control period (update at ~200 Hz)
    control_period_samples_ = static_cast<uint32_t>(sample_rate / 200.0f);
    samples_until_control_update_ = 0;

    // Initialize resonator parameters
    updateResonatorParameters();

    // Reset state
    reset();
}

void ResonantBodyProcessor::setBodySize(float size) {
    body_size_ = std::clamp(size, 0.0f, 1.0f);
    frequency_scale_ = mapBodySizeToFrequencyScale(body_size_);
}

void ResonantBodyProcessor::setMaterial(float material) {
    material_ = std::clamp(material, 0.0f, 1.0f);
}

void ResonantBodyProcessor::setExcitation(float excite) {
    excitation_ = std::clamp(excite, 0.0f, 1.0f);
}

void ResonantBodyProcessor::setMorph(float morph) {
    morph_ = std::clamp(morph, 0.0f, 1.0f);
}

void ResonantBodyProcessor::setMix(float mix) {
    mix_ = std::clamp(mix, 0.0f, 1.0f);
}

void ResonantBodyProcessor::setBaseFrequency(float freq_hz) {
    base_frequency_ = std::clamp(freq_hz, 20.0f, 10000.0f);
}

float ResonantBodyProcessor::mapBodySizeToFrequencyScale(float size) const {
    // Map 0-1 to frequency scale 4.0 - 0.25 (inverse relationship)
    // Small body (0.0) = high pitch (4x)
    // Large body (1.0) = low pitch (0.25x)
    //
    // Using exponential mapping for musical scaling
    const float min_scale = 0.25f;
    const float max_scale = 4.0f;

    // Exponential interpolation: scale = max * (min/max)^size
    return max_scale * std::pow(min_scale / max_scale, size);
}

float ResonantBodyProcessor::mapMaterialToDamping(float material) const {
    // Map 0-1 to damping 5.0 - 0.1 (inverse relationship)
    // Soft material (0.0) = high damping (5.0) = short decay
    // Hard material (1.0) = low damping (0.1) = long ring
    //
    // Using exponential mapping for perceptually uniform decay
    const float min_damping = 0.1f;
    const float max_damping = 5.0f;

    return max_damping * std::pow(min_damping / max_damping, material);
}

void ResonantBodyProcessor::updateResonatorParameters() {
    // Calculate damping from material parameter
    const float base_damping = mapMaterialToDamping(material_);

    // Configure each mode
    for (int k = 0; k < MAX_MODES; k++) {
        // Mode frequency: base * multiplier * scale
        const float mode_freq_hz = base_frequency_ * mode_freq_multipliers_[k] * frequency_scale_;
        const float omega = 2.0f * M_PI * mode_freq_hz;

        // Mode damping: higher modes decay faster (more realistic)
        // Add slight increase in damping for higher modes
        const float mode_damping_mult = 1.0f + 0.2f * static_cast<float>(k);
        const float gamma = base_damping * mode_damping_mult;

        // Mode weight: all modes contribute equally by default
        // Could vary this for different timbres
        const float weight = 0.25f;  // 4 modes, each 0.25 for normalized output

        // Set mode parameters
        modal_node_set_mode(&modal_node_, k, omega, gamma, weight);

        // Activate mode
        modal_node_.modes[k].params.active = true;

        // Set wave shape (pure sine for clean resonance)
        modal_node_.modes[k].params.shape = WAVE_SHAPE_SINE;
    }

    // Set audio output gain
    modal_node_.audio_gain = 1.0f;
}

void ResonantBodyProcessor::applyPitchMorph(float detected_pitch, float confidence) {
    if (detected_pitch < 20.0f || confidence < 0.3f) {
        return;  // No confident pitch detected
    }

    // Morph rate: very slow drift (like thermal expansion)
    // At morph = 1.0, frequency moves ~1% toward target per control update
    // At 200 Hz control rate, this means full convergence takes ~0.5 seconds
    const float morph_rate = 0.01f * morph_ * confidence;

    // Slowly drift base frequency toward detected pitch
    const float frequency_error = detected_pitch - base_frequency_;
    base_frequency_ += morph_rate * frequency_error;

    // Clamp to reasonable range
    base_frequency_ = std::clamp(base_frequency_, 60.0f, 2000.0f);
}

void ResonantBodyProcessor::injectEnergy(float energy, const float band_energies[3]) {
    if (energy < 1e-6f) {
        return;  // Silence, no excitation
    }

    // Scale energy by excitation parameter
    const float scaled_energy = energy * excitation_;

    if (scaled_energy < 1e-6f) {
        return;  // Excitation turned down
    }

    // Create poke event
    poke_event_t poke;
    poke.source_node_id = 0;    // Source node ID (not used in effect mode)
    poke.strength = scaled_energy;
    poke.phase_hint = 0.0f;     // No phase hint (random initial phase)

    // Distribute energy across modes based on spectral content
    // Map 3 bands to 4 modes with simple weighting
    //
    // Strategy:
    //   - Low band (20-400 Hz) excites modes 0-1 (fundamental, first harmonic)
    //   - Mid band (400-3k Hz) excites modes 1-2 (harmonics)
    //   - High band (3k-20k Hz) excites modes 2-3 (high harmonics)

    const float low_energy = band_energies[SpectralAnalyzer::LOW];
    const float mid_energy = band_energies[SpectralAnalyzer::MID];
    const float high_energy = band_energies[SpectralAnalyzer::HIGH];

    // Weight modes by spectral content
    poke.mode_weights[0] = 0.7f * low_energy + 0.3f * mid_energy;  // Fundamental
    poke.mode_weights[1] = 0.3f * low_energy + 0.5f * mid_energy;  // First harmonic
    poke.mode_weights[2] = 0.2f * mid_energy + 0.6f * high_energy; // Second harmonic
    poke.mode_weights[3] = 0.4f * high_energy;                     // Third harmonic

    // Normalize weights so sum = 1.0
    float weight_sum = poke.mode_weights[0] + poke.mode_weights[1] +
                       poke.mode_weights[2] + poke.mode_weights[3];

    if (weight_sum > 1e-6f) {
        for (int k = 0; k < MAX_MODES; k++) {
            poke.mode_weights[k] /= weight_sum;
        }
    } else {
        // Fallback: equal weights
        for (int k = 0; k < MAX_MODES; k++) {
            poke.mode_weights[k] = 0.25f;
        }
    }

    // Apply poke to modal node
    modal_node_apply_poke(&modal_node_, &poke);
}

void ResonantBodyProcessor::process(const float* inputL, const float* inputR,
                                     float* outputL, float* outputR,
                                     uint32_t num_frames) {
    // Convert stereo input to mono for analysis (simple average)
    // TODO: Could use a persistent mono buffer to avoid allocation
    std::vector<float> mono_input(num_frames);
    for (uint32_t i = 0; i < num_frames; i++) {
        mono_input[i] = (inputL[i] + inputR[i]) * 0.5f;
    }

    // Extract energy and spectral content
    const float energy = energy_extractor_.process(mono_input.data(), num_frames);

    float band_energies[SpectralAnalyzer::NUM_BANDS];
    spectral_analyzer_.process(mono_input.data(), num_frames, band_energies);

    // Update pitch detection (if morphing enabled)
    if (morph_ > 0.01f) {
        pitch_detector_.process(mono_input.data(), num_frames);
    }

    // Control rate updates (parameter changes, pitch morphing)
    samples_until_control_update_ += num_frames;
    if (samples_until_control_update_ >= control_period_samples_) {
        samples_until_control_update_ = 0;

        // Apply pitch morphing if enabled
        if (morph_ > 0.01f) {
            const float detected_pitch = pitch_detector_.getPitch();
            const float confidence = pitch_detector_.getConfidence();
            applyPitchMorph(detected_pitch, confidence);
        }

        // Update resonator parameters
        updateResonatorParameters();

        // Update modal physics (call at control rate)
        modal_node_step(&modal_node_);
    }

    // Inject energy into resonator
    injectEnergy(energy, band_energies);

    // Render resonator audio
    std::vector<float> wet_L(num_frames);
    std::vector<float> wet_R(num_frames);
    audio_synth_render(&audio_synth_, wet_L.data(), wet_R.data(), num_frames);

    // Mix dry and wet signals
    for (uint32_t i = 0; i < num_frames; i++) {
        outputL[i] = (1.0f - mix_) * inputL[i] + mix_ * wet_L[i];
        outputR[i] = (1.0f - mix_) * inputR[i] + mix_ * wet_R[i];
    }
}

float ResonantBodyProcessor::getResonatorEnergy() const {
    return modal_node_get_amplitude(&modal_node_);
}

void ResonantBodyProcessor::reset() {
    // Reset modal node state (zero all mode amplitudes)
    for (int k = 0; k < MAX_MODES; k++) {
        modal_node_.modes[k].a = 0.0f;
        modal_node_.modes[k].a_dot = 0.0f;
    }

    // Reset analysis components
    energy_extractor_.reset();
    spectral_analyzer_.reset();
    pitch_detector_.reset();

    // Reset control timing
    samples_until_control_update_ = 0;
}
