/**
 * @file PitchDetector.cpp
 * @brief Autocorrelation-based pitch detection implementation
 */

#include "PitchDetector.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Helper function to calculate smoothing coefficient from time constant
static float calculate_smoothing_coeff(float time_ms, float sample_rate) {
    return expf(-1000.0f / (time_ms * sample_rate));
}

// Autocorrelation function
static float autocorrelate(const float* buffer, uint32_t size, uint32_t lag) {
    float sum = 0.0f;
    for (uint32_t i = 0; i < size - lag; i++) {
        sum += buffer[i] * buffer[i + lag];
    }
    return sum;
}

void pitch_detector_init(pitch_detector_t* detector,
                        float sample_rate,
                        float min_freq,
                        float max_freq,
                        float buffer_size_ms,
                        float smoothing_ms) {
    if (!detector) return;

    detector->sample_rate = sample_rate;
    detector->min_freq = min_freq;
    detector->max_freq = max_freq;

    // Calculate buffer size
    detector->buffer_size = (uint32_t)(buffer_size_ms * sample_rate / 1000.0f);
    if (detector->buffer_size < 64) {
        detector->buffer_size = 64;
    }

    // Allocate buffer
    detector->buffer = (float*)calloc(detector->buffer_size, sizeof(float));
    detector->buffer_index = 0;

    detector->detected_pitch = 0.0f;
    detector->smoothed_pitch = 0.0f;
    detector->confidence = 0.0f;
    detector->pitch_valid = false;

    // Calculate smoothing coefficient
    detector->smoothing_coeff = calculate_smoothing_coeff(smoothing_ms, sample_rate);

    detector->initialized = (detector->buffer != NULL);
}

void pitch_detector_process(pitch_detector_t* detector, float input) {
    if (!detector || !detector->initialized) return;

    // Add sample to circular buffer
    detector->buffer[detector->buffer_index] = input;
    detector->buffer_index = (detector->buffer_index + 1) % detector->buffer_size;
}

void pitch_detector_process_buffer(pitch_detector_t* detector,
                                   const float* input,
                                   uint32_t num_samples) {
    if (!detector || !input) return;

    for (uint32_t i = 0; i < num_samples; i++) {
        pitch_detector_process(detector, input[i]);
    }
}

void pitch_detector_analyze(pitch_detector_t* detector) {
    if (!detector || !detector->initialized) return;

    // Calculate min and max lag from frequency range
    uint32_t min_lag = (uint32_t)(detector->sample_rate / detector->max_freq);
    uint32_t max_lag = (uint32_t)(detector->sample_rate / detector->min_freq);

    // Ensure lags are within buffer bounds
    if (min_lag < 1) min_lag = 1;
    if (max_lag >= detector->buffer_size) max_lag = detector->buffer_size - 1;

    // Calculate autocorrelation at lag 0 for normalization
    float r0 = autocorrelate(detector->buffer, detector->buffer_size, 0);
    if (r0 < 1e-6f) {
        // Signal too quiet
        detector->pitch_valid = false;
        detector->confidence = 0.0f;
        return;
    }

    // Find maximum autocorrelation peak
    float max_correlation = -1.0f;
    uint32_t best_lag = 0;

    for (uint32_t lag = min_lag; lag <= max_lag; lag++) {
        float correlation = autocorrelate(detector->buffer, detector->buffer_size, lag);
        float normalized = correlation / r0;

        if (normalized > max_correlation) {
            max_correlation = normalized;
            best_lag = lag;
        }
    }

    // Check if peak is significant enough
    const float CONFIDENCE_THRESHOLD = 0.3f;
    if (max_correlation > CONFIDENCE_THRESHOLD && best_lag > 0) {
        // Valid pitch detected
        detector->detected_pitch = detector->sample_rate / (float)best_lag;
        detector->confidence = max_correlation;
        detector->pitch_valid = true;

        // Apply smoothing
        detector->smoothed_pitch = detector->smoothing_coeff * detector->smoothed_pitch +
                                  (1.0f - detector->smoothing_coeff) * detector->detected_pitch;
    } else {
        // No valid pitch
        detector->pitch_valid = false;
        detector->confidence = max_correlation;
        // Keep smoothed pitch but decay confidence
    }
}

float pitch_detector_get_pitch(const pitch_detector_t* detector) {
    if (!detector || !detector->pitch_valid) return 0.0f;
    return detector->detected_pitch;
}

float pitch_detector_get_smoothed_pitch(const pitch_detector_t* detector) {
    if (!detector || !detector->pitch_valid) return 0.0f;
    return detector->smoothed_pitch;
}

float pitch_detector_get_confidence(const pitch_detector_t* detector) {
    if (!detector) return 0.0f;
    return detector->confidence;
}

bool pitch_detector_is_valid(const pitch_detector_t* detector) {
    if (!detector) return false;
    return detector->pitch_valid;
}

void pitch_detector_reset(pitch_detector_t* detector) {
    if (!detector || !detector->initialized) return;

    detector->buffer_index = 0;
    detector->detected_pitch = 0.0f;
    detector->smoothed_pitch = 0.0f;
    detector->confidence = 0.0f;
    detector->pitch_valid = false;

    if (detector->buffer) {
        memset(detector->buffer, 0, detector->buffer_size * sizeof(float));
    }
}

void pitch_detector_set_range(pitch_detector_t* detector,
                             float min_freq,
                             float max_freq) {
    if (!detector) return;
    detector->min_freq = min_freq;
    detector->max_freq = max_freq;
}

void pitch_detector_cleanup(pitch_detector_t* detector) {
    if (!detector) return;

    if (detector->buffer) {
        free(detector->buffer);
        detector->buffer = NULL;
    }

    detector->initialized = false;
}
