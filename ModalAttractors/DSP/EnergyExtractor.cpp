//
//  EnergyExtractor.cpp
//  ModalAttractors
//
//  Energy extraction from audio input for resonant body excitation.
//
//  Created by Claude on 2026-01-11.
//

#include "EnergyExtractor.h"
#include <cmath>
#include <algorithm>

EnergyExtractor::EnergyExtractor()
    : sample_rate_(48000.0f)
    , envelope_(0.0f)
    , attack_coeff_(0.0f)
    , release_coeff_(0.0f)
    , attack_time_ms_(5.0f)      // Fast attack for transients
    , release_time_ms_(100.0f)   // Moderate release for sustain
{
    updateCoefficients();
}

void EnergyExtractor::initialize(float sample_rate) {
    sample_rate_ = sample_rate;
    envelope_ = 0.0f;
    updateCoefficients();
}

void EnergyExtractor::setAttackTime(float attack_ms) {
    attack_time_ms_ = std::max(0.1f, attack_ms);  // Minimum 0.1ms
    updateCoefficients();
}

void EnergyExtractor::setReleaseTime(float release_ms) {
    release_time_ms_ = std::max(1.0f, release_ms);  // Minimum 1ms
    updateCoefficients();
}

void EnergyExtractor::updateCoefficients() {
    // Convert time constants to exponential smoothing coefficients
    // For a time constant tau, the coefficient is: 1 - exp(-dt/tau)
    // where dt = 1/sample_rate
    //
    // Simplified: coeff = 1 - exp(-1000 / (time_ms * sample_rate))

    const float attack_samples = (attack_time_ms_ / 1000.0f) * sample_rate_;
    const float release_samples = (release_time_ms_ / 1000.0f) * sample_rate_;

    // Exponential smoothing coefficient
    // For fast convergence: coeff ≈ 1/N where N is time in samples
    // Using more accurate exponential formula
    attack_coeff_ = 1.0f - std::exp(-1.0f / attack_samples);
    release_coeff_ = 1.0f - std::exp(-1.0f / release_samples);

    // Clamp to reasonable values
    attack_coeff_ = std::clamp(attack_coeff_, 0.0001f, 1.0f);
    release_coeff_ = std::clamp(release_coeff_, 0.0001f, 1.0f);
}

float EnergyExtractor::computeRMS(const float* input, uint32_t num_frames) {
    if (num_frames == 0) return 0.0f;

    // Compute sum of squares
    float sum_squares = 0.0f;
    for (uint32_t i = 0; i < num_frames; i++) {
        sum_squares += input[i] * input[i];
    }

    // Return RMS (root mean square)
    return std::sqrt(sum_squares / static_cast<float>(num_frames));
}

float EnergyExtractor::process(const float* input, uint32_t num_frames) {
    // Compute instantaneous RMS of this block
    float block_rms = computeRMS(input, num_frames);

    // Apply attack/release envelope following
    // If RMS is rising (attack), use fast attack coefficient
    // If RMS is falling (release), use slower release coefficient
    float coeff = (block_rms > envelope_) ? attack_coeff_ : release_coeff_;

    // Exponential smoothing: envelope += coeff * (target - envelope)
    envelope_ += coeff * (block_rms - envelope_);

    // Clamp to prevent numerical drift
    envelope_ = std::max(0.0f, envelope_);

    return envelope_;
}

void EnergyExtractor::reset() {
    envelope_ = 0.0f;
}
