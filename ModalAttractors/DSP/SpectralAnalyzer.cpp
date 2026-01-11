//
//  SpectralAnalyzer.cpp
//  ModalAttractors
//
//  Multi-band spectral analysis for resonant body excitation weighting.
//
//  Created by Claude on 2026-01-11.
//

#include "SpectralAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// BiquadFilter Implementation
// ============================================================================

void BiquadFilter::setLowpass(float sample_rate, float cutoff_freq, float q) {
    const float omega = 2.0f * M_PI * cutoff_freq / sample_rate;
    const float cos_omega = std::cos(omega);
    const float sin_omega = std::sin(omega);
    const float alpha = sin_omega / (2.0f * q);

    const float a0 = 1.0f + alpha;
    b0 = ((1.0f - cos_omega) / 2.0f) / a0;
    b1 = (1.0f - cos_omega) / a0;
    b2 = ((1.0f - cos_omega) / 2.0f) / a0;
    a1 = (-2.0f * cos_omega) / a0;
    a2 = (1.0f - alpha) / a0;
}

void BiquadFilter::setHighpass(float sample_rate, float cutoff_freq, float q) {
    const float omega = 2.0f * M_PI * cutoff_freq / sample_rate;
    const float cos_omega = std::cos(omega);
    const float sin_omega = std::sin(omega);
    const float alpha = sin_omega / (2.0f * q);

    const float a0 = 1.0f + alpha;
    b0 = ((1.0f + cos_omega) / 2.0f) / a0;
    b1 = -(1.0f + cos_omega) / a0;
    b2 = ((1.0f + cos_omega) / 2.0f) / a0;
    a1 = (-2.0f * cos_omega) / a0;
    a2 = (1.0f - alpha) / a0;
}

void BiquadFilter::setBandpass(float sample_rate, float center_freq, float bandwidth) {
    const float omega = 2.0f * M_PI * center_freq / sample_rate;
    const float cos_omega = std::cos(omega);
    const float sin_omega = std::sin(omega);
    const float q = center_freq / bandwidth;
    const float alpha = sin_omega / (2.0f * q);

    const float a0 = 1.0f + alpha;
    b0 = alpha / a0;
    b1 = 0.0f;
    b2 = -alpha / a0;
    a1 = (-2.0f * cos_omega) / a0;
    a2 = (1.0f - alpha) / a0;
}

// ============================================================================
// SpectralAnalyzer Implementation
// ============================================================================

SpectralAnalyzer::SpectralAnalyzer()
    : sample_rate_(48000.0f)
    , smoothing_coeff_(0.1f)
    , smoothing_time_ms_(20.0f)
{
    std::memset(band_rms_, 0, sizeof(band_rms_));
}

void SpectralAnalyzer::initialize(float sample_rate) {
    sample_rate_ = sample_rate;

    // Configure filter bank
    // Band edges: [20 Hz - 400 Hz] | [400 Hz - 3 kHz] | [3 kHz - 20 kHz]

    // Low band: Lowpass @ 400 Hz
    low_filter_.setLowpass(sample_rate, 400.0f, 0.707f);

    // Mid band: Bandpass 400 Hz - 3 kHz
    // Implemented as highpass @ 400 Hz + lowpass @ 3 kHz
    mid_filter_low_.setHighpass(sample_rate, 400.0f, 0.707f);
    mid_filter_high_.setLowpass(sample_rate, 3000.0f, 0.707f);

    // High band: Highpass @ 3 kHz
    high_filter_.setHighpass(sample_rate, 3000.0f, 0.707f);

    // Update smoothing coefficient
    updateSmoothingCoeff();

    // Reset state
    reset();
}

void SpectralAnalyzer::setSmoothingTime(float smoothing_ms) {
    smoothing_time_ms_ = std::max(1.0f, smoothing_ms);
    updateSmoothingCoeff();
}

void SpectralAnalyzer::updateSmoothingCoeff() {
    const float smoothing_samples = (smoothing_time_ms_ / 1000.0f) * sample_rate_;
    smoothing_coeff_ = 1.0f - std::exp(-1.0f / smoothing_samples);
    smoothing_coeff_ = std::clamp(smoothing_coeff_, 0.0001f, 1.0f);
}

float SpectralAnalyzer::computeRMS(const float* input, uint32_t num_frames) {
    if (num_frames == 0) return 0.0f;

    float sum_squares = 0.0f;
    for (uint32_t i = 0; i < num_frames; i++) {
        sum_squares += input[i] * input[i];
    }

    return std::sqrt(sum_squares / static_cast<float>(num_frames));
}

void SpectralAnalyzer::process(const float* input, uint32_t num_frames, float band_energies[NUM_BANDS]) {
    // Temporary buffers for filtered audio
    // Note: For efficiency, we could use pre-allocated buffers in real-time code
    // For now, we'll process sample-by-sample and accumulate RMS

    float low_sum_sq = 0.0f;
    float mid_sum_sq = 0.0f;
    float high_sum_sq = 0.0f;

    for (uint32_t i = 0; i < num_frames; i++) {
        const float sample = input[i];

        // Process through filter bank
        const float low_sample = low_filter_.process(sample);
        const float high_sample = high_filter_.process(sample);

        // Mid band: cascade highpass + lowpass
        float mid_sample = mid_filter_low_.process(sample);
        mid_sample = mid_filter_high_.process(mid_sample);

        // Accumulate squared samples for RMS
        low_sum_sq += low_sample * low_sample;
        mid_sum_sq += mid_sample * mid_sample;
        high_sum_sq += high_sample * high_sample;
    }

    // Compute instantaneous RMS per band
    const float low_rms = std::sqrt(low_sum_sq / static_cast<float>(num_frames));
    const float mid_rms = std::sqrt(mid_sum_sq / static_cast<float>(num_frames));
    const float high_rms = std::sqrt(high_sum_sq / static_cast<float>(num_frames));

    // Apply smoothing (exponential moving average)
    band_rms_[LOW] += smoothing_coeff_ * (low_rms - band_rms_[LOW]);
    band_rms_[MID] += smoothing_coeff_ * (mid_rms - band_rms_[MID]);
    band_rms_[HIGH] += smoothing_coeff_ * (high_rms - band_rms_[HIGH]);

    // Clamp to prevent drift
    band_rms_[LOW] = std::max(0.0f, band_rms_[LOW]);
    band_rms_[MID] = std::max(0.0f, band_rms_[MID]);
    band_rms_[HIGH] = std::max(0.0f, band_rms_[HIGH]);

    // Output current band energies
    band_energies[LOW] = band_rms_[LOW];
    band_energies[MID] = band_rms_[MID];
    band_energies[HIGH] = band_rms_[HIGH];
}

void SpectralAnalyzer::getBandEnergies(float band_energies[NUM_BANDS]) const {
    band_energies[LOW] = band_rms_[LOW];
    band_energies[MID] = band_rms_[MID];
    band_energies[HIGH] = band_rms_[HIGH];
}

void SpectralAnalyzer::reset() {
    low_filter_.reset();
    mid_filter_low_.reset();
    mid_filter_high_.reset();
    high_filter_.reset();

    std::memset(band_rms_, 0, sizeof(band_rms_));
}
