//
//  PitchDetector.cpp
//  ModalAttractors
//
//  Simple pitch detection for resonant body frequency morphing.
//
//  Created by Claude on 2026-01-11.
//

#include "PitchDetector.h"
#include <cmath>
#include <algorithm>
#include <cstring>

PitchDetector::PitchDetector()
    : sample_rate_(48000.0f)
    , min_pitch_hz_(60.0f)      // About B1
    , max_pitch_hz_(2000.0f)    // About B6
    , window_size_ms_(40.0f)    // 40ms window
    , window_samples_(0)
    , estimated_pitch_hz_(0.0f)
    , confidence_(0.0f)
    , buffer_write_pos_(0)
{
}

void PitchDetector::initialize(float sample_rate) {
    sample_rate_ = sample_rate;

    // Calculate window size in samples
    window_samples_ = static_cast<uint32_t>((window_size_ms_ / 1000.0f) * sample_rate);

    // Allocate audio buffer (circular buffer)
    audio_buffer_.resize(window_samples_, 0.0f);
    buffer_write_pos_ = 0;

    // Reset state
    estimated_pitch_hz_ = 0.0f;
    confidence_ = 0.0f;
}

void PitchDetector::setWindowSize(float window_ms) {
    window_size_ms_ = std::max(10.0f, window_ms);

    // Recalculate window samples
    window_samples_ = static_cast<uint32_t>((window_size_ms_ / 1000.0f) * sample_rate_);

    // Resize buffer
    audio_buffer_.resize(window_samples_, 0.0f);
    buffer_write_pos_ = 0;
}

void PitchDetector::process(const float* input, uint32_t num_frames) {
    // Copy input into circular buffer
    for (uint32_t i = 0; i < num_frames; i++) {
        audio_buffer_[buffer_write_pos_] = input[i];
        buffer_write_pos_ = (buffer_write_pos_ + 1) % window_samples_;
    }

    // Run pitch detection every time buffer fills up
    // This means we update pitch estimate every ~40ms (window_size_ms_)
    // You could also run it less frequently for efficiency
    static uint32_t frame_counter = 0;
    frame_counter += num_frames;

    // Update every ~40ms worth of samples
    if (frame_counter >= window_samples_) {
        runPitchDetection();
        frame_counter = 0;
    }
}

float PitchDetector::computeAutocorrelation(const float* buffer, uint32_t buffer_size, uint32_t lag) {
    if (lag >= buffer_size) return 0.0f;

    // Compute autocorrelation: sum(x[n] * x[n + lag])
    float correlation = 0.0f;
    const uint32_t count = buffer_size - lag;

    for (uint32_t i = 0; i < count; i++) {
        correlation += buffer[i] * buffer[i + lag];
    }

    // Normalize by buffer energy (to get values in [0, 1])
    // Using energy of the segment being correlated
    float energy = 0.0f;
    for (uint32_t i = 0; i < count; i++) {
        energy += buffer[i] * buffer[i];
    }

    if (energy < 1e-6f) return 0.0f;  // Silence

    return correlation / energy;
}

void PitchDetector::runPitchDetection() {
    // Linearize circular buffer for easier processing
    std::vector<float> linear_buffer(window_samples_);
    for (uint32_t i = 0; i < window_samples_; i++) {
        linear_buffer[i] = audio_buffer_[(buffer_write_pos_ + i) % window_samples_];
    }

    // Calculate lag range based on pitch range
    const uint32_t min_lag = static_cast<uint32_t>(sample_rate_ / max_pitch_hz_);
    const uint32_t max_lag = static_cast<uint32_t>(sample_rate_ / min_pitch_hz_);

    // Clamp to buffer size
    const uint32_t safe_max_lag = std::min(max_lag, window_samples_ / 2);

    if (min_lag >= safe_max_lag) {
        // Invalid range, no pitch detected
        estimated_pitch_hz_ = 0.0f;
        confidence_ = 0.0f;
        return;
    }

    // Find lag with maximum autocorrelation
    float max_correlation = 0.0f;
    uint32_t best_lag = min_lag;

    for (uint32_t lag = min_lag; lag <= safe_max_lag; lag++) {
        float correlation = computeAutocorrelation(linear_buffer.data(), window_samples_, lag);

        if (correlation > max_correlation) {
            max_correlation = correlation;
            best_lag = lag;
        }
    }

    // Check if we found a strong enough correlation
    // Typical threshold for voiced speech: 0.3-0.5
    // For musical input, we can be more lenient
    const float confidence_threshold = 0.2f;

    if (max_correlation > confidence_threshold) {
        // Convert lag to frequency
        estimated_pitch_hz_ = sample_rate_ / static_cast<float>(best_lag);
        confidence_ = std::min(1.0f, max_correlation);  // Clamp to [0, 1]
    } else {
        // No confident pitch detected
        estimated_pitch_hz_ = 0.0f;
        confidence_ = 0.0f;
    }
}

void PitchDetector::reset() {
    std::fill(audio_buffer_.begin(), audio_buffer_.end(), 0.0f);
    buffer_write_pos_ = 0;
    estimated_pitch_hz_ = 0.0f;
    confidence_ = 0.0f;
}
