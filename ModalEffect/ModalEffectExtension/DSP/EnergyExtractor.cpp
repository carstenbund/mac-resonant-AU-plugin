/**
 * @file EnergyExtractor.cpp
 * @brief RMS energy analysis with attack/release envelope implementation
 */

#include "EnergyExtractor.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Helper function to calculate envelope coefficient from time constant
static float calculate_time_constant(float time_ms, float sample_rate) {
    // exp(-1/(time_constant * sample_rate))
    // For a given time in ms, calculate the coefficient for exponential smoothing
    return expf(-1000.0f / (time_ms * sample_rate));
}

void energy_extractor_init(energy_extractor_t* extractor,
                          float sample_rate,
                          float attack_ms,
                          float release_ms,
                          float rms_window_ms) {
    if (!extractor) return;

    extractor->sample_rate = sample_rate;
    extractor->envelope = 0.0f;

    // Calculate attack and release coefficients
    extractor->attack_coeff = calculate_time_constant(attack_ms, sample_rate);
    extractor->release_coeff = calculate_time_constant(release_ms, sample_rate);

    // Calculate RMS window size
    extractor->rms_window_size = (uint32_t)(rms_window_ms * sample_rate / 1000.0f);
    if (extractor->rms_window_size < 1) {
        extractor->rms_window_size = 1;
    }

    // Allocate RMS buffer
    extractor->rms_buffer = (float*)calloc(extractor->rms_window_size, sizeof(float));
    extractor->rms_window_sum = 0.0f;
    extractor->rms_index = 0;

    extractor->initialized = (extractor->rms_buffer != NULL);
}

float energy_extractor_process(energy_extractor_t* extractor, float input) {
    if (!extractor || !extractor->initialized) return 0.0f;

    // Calculate squared input
    float input_sq = input * input;

    // Update RMS window (circular buffer)
    float old_sample = extractor->rms_buffer[extractor->rms_index];
    extractor->rms_buffer[extractor->rms_index] = input_sq;

    // Update running sum
    extractor->rms_window_sum = extractor->rms_window_sum - old_sample + input_sq;

    // Advance circular buffer index
    extractor->rms_index = (extractor->rms_index + 1) % extractor->rms_window_size;

    // Calculate RMS value
    float rms = sqrtf(extractor->rms_window_sum / (float)extractor->rms_window_size);

    // Apply attack/release envelope follower
    float coeff;
    if (rms > extractor->envelope) {
        // Attack (rising)
        coeff = extractor->attack_coeff;
    } else {
        // Release (falling)
        coeff = extractor->release_coeff;
    }

    extractor->envelope = coeff * extractor->envelope + (1.0f - coeff) * rms;

    return extractor->envelope;
}

void energy_extractor_process_buffer(energy_extractor_t* extractor,
                                     const float* input,
                                     float* output,
                                     uint32_t num_samples) {
    if (!extractor || !input || !output) return;

    for (uint32_t i = 0; i < num_samples; i++) {
        output[i] = energy_extractor_process(extractor, input[i]);
    }
}

float energy_extractor_get_envelope(const energy_extractor_t* extractor) {
    if (!extractor) return 0.0f;
    return extractor->envelope;
}

void energy_extractor_reset(energy_extractor_t* extractor) {
    if (!extractor || !extractor->initialized) return;

    extractor->envelope = 0.0f;
    extractor->rms_window_sum = 0.0f;
    extractor->rms_index = 0;

    if (extractor->rms_buffer) {
        memset(extractor->rms_buffer, 0, extractor->rms_window_size * sizeof(float));
    }
}

void energy_extractor_set_attack(energy_extractor_t* extractor, float attack_ms) {
    if (!extractor) return;
    extractor->attack_coeff = calculate_time_constant(attack_ms, extractor->sample_rate);
}

void energy_extractor_set_release(energy_extractor_t* extractor, float release_ms) {
    if (!extractor) return;
    extractor->release_coeff = calculate_time_constant(release_ms, extractor->sample_rate);
}

void energy_extractor_cleanup(energy_extractor_t* extractor) {
    if (!extractor) return;

    if (extractor->rms_buffer) {
        free(extractor->rms_buffer);
        extractor->rms_buffer = NULL;
    }

    extractor->initialized = false;
}
