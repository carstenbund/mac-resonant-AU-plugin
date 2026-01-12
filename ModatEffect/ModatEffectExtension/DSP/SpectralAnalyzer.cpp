/**
 * @file SpectralAnalyzer.cpp
 * @brief 3-band biquad filter bank implementation
 */

#include "SpectralAnalyzer.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Biquad Filter Utilities
// ============================================================================

static void biquad_reset(biquad_t* filter) {
    if (!filter) return;
    filter->x1 = filter->x2 = 0.0f;
    filter->y1 = filter->y2 = 0.0f;
}

static float biquad_process(biquad_t* filter, float input) {
    if (!filter) return input;

    // Direct Form II Transposed
    float output = filter->b0 * input + filter->b1 * filter->x1 + filter->b2 * filter->x2
                   - filter->a1 * filter->y1 - filter->a2 * filter->y2;

    // Update state
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;

    return output;
}

// Configure biquad as 2nd-order Butterworth lowpass
static void biquad_lowpass(biquad_t* filter, float cutoff_hz, float sample_rate) {
    if (!filter) return;

    float omega = 2.0f * M_PI * cutoff_hz / sample_rate;
    float cos_omega = cosf(omega);
    float sin_omega = sinf(omega);
    float alpha = sin_omega / (2.0f * 0.707f); // Q = 0.707 for Butterworth

    float a0 = 1.0f + alpha;
    filter->a1 = (-2.0f * cos_omega) / a0;
    filter->a2 = (1.0f - alpha) / a0;

    filter->b0 = ((1.0f - cos_omega) / 2.0f) / a0;
    filter->b1 = (1.0f - cos_omega) / a0;
    filter->b2 = ((1.0f - cos_omega) / 2.0f) / a0;

    biquad_reset(filter);
}

// Configure biquad as 2nd-order Butterworth highpass
static void biquad_highpass(biquad_t* filter, float cutoff_hz, float sample_rate) {
    if (!filter) return;

    float omega = 2.0f * M_PI * cutoff_hz / sample_rate;
    float cos_omega = cosf(omega);
    float sin_omega = sinf(omega);
    float alpha = sin_omega / (2.0f * 0.707f); // Q = 0.707 for Butterworth

    float a0 = 1.0f + alpha;
    filter->a1 = (-2.0f * cos_omega) / a0;
    filter->a2 = (1.0f - alpha) / a0;

    filter->b0 = ((1.0f + cos_omega) / 2.0f) / a0;
    filter->b1 = -(1.0f + cos_omega) / a0;
    filter->b2 = ((1.0f + cos_omega) / 2.0f) / a0;

    biquad_reset(filter);
}

// Configure biquad as 2nd-order bandpass
static void biquad_bandpass(biquad_t* filter, float center_hz, float bandwidth, float sample_rate) {
    if (!filter) return;

    float omega = 2.0f * M_PI * center_hz / sample_rate;
    float cos_omega = cosf(omega);
    float sin_omega = sinf(omega);
    float Q = center_hz / bandwidth;
    float alpha = sin_omega / (2.0f * Q);

    float a0 = 1.0f + alpha;
    filter->a1 = (-2.0f * cos_omega) / a0;
    filter->a2 = (1.0f - alpha) / a0;

    filter->b0 = alpha / a0;
    filter->b1 = 0.0f;
    filter->b2 = -alpha / a0;

    biquad_reset(filter);
}

// ============================================================================
// Spectral Analyzer Implementation
// ============================================================================

void spectral_analyzer_init(spectral_analyzer_t* analyzer,
                           float sample_rate,
                           float crossover_low,
                           float crossover_high) {
    if (!analyzer) return;

    analyzer->sample_rate = sample_rate;
    analyzer->crossover_low = crossover_low;
    analyzer->crossover_high = crossover_high;

    // Configure filters
    biquad_lowpass(&analyzer->low_pass, crossover_low, sample_rate);

    // Bandpass centered between crossovers
    float center_freq = sqrtf(crossover_low * crossover_high);
    float bandwidth = crossover_high - crossover_low;
    biquad_bandpass(&analyzer->band_pass, center_freq, bandwidth, sample_rate);

    biquad_highpass(&analyzer->high_pass, crossover_high, sample_rate);

    analyzer->initialized = true;
}

void spectral_analyzer_process(spectral_analyzer_t* analyzer,
                              float input,
                              float band_outputs[NUM_BANDS]) {
    if (!analyzer || !analyzer->initialized || !band_outputs) return;

    band_outputs[BAND_LOW] = biquad_process(&analyzer->low_pass, input);
    band_outputs[BAND_MID] = biquad_process(&analyzer->band_pass, input);
    band_outputs[BAND_HIGH] = biquad_process(&analyzer->high_pass, input);
}

void spectral_analyzer_process_buffer(spectral_analyzer_t* analyzer,
                                      const float* input,
                                      float* low_output,
                                      float* mid_output,
                                      float* high_output,
                                      uint32_t num_samples) {
    if (!analyzer || !input || !low_output || !mid_output || !high_output) return;

    for (uint32_t i = 0; i < num_samples; i++) {
        float band_outputs[NUM_BANDS];
        spectral_analyzer_process(analyzer, input[i], band_outputs);

        low_output[i] = band_outputs[BAND_LOW];
        mid_output[i] = band_outputs[BAND_MID];
        high_output[i] = band_outputs[BAND_HIGH];
    }
}

void spectral_analyzer_reset(spectral_analyzer_t* analyzer) {
    if (!analyzer) return;

    biquad_reset(&analyzer->low_pass);
    biquad_reset(&analyzer->band_pass);
    biquad_reset(&analyzer->high_pass);
}

void spectral_analyzer_set_crossovers(spectral_analyzer_t* analyzer,
                                     float crossover_low,
                                     float crossover_high) {
    if (!analyzer || !analyzer->initialized) return;

    analyzer->crossover_low = crossover_low;
    analyzer->crossover_high = crossover_high;

    // Reconfigure filters
    biquad_lowpass(&analyzer->low_pass, crossover_low, analyzer->sample_rate);

    float center_freq = sqrtf(crossover_low * crossover_high);
    float bandwidth = crossover_high - crossover_low;
    biquad_bandpass(&analyzer->band_pass, center_freq, bandwidth, analyzer->sample_rate);

    biquad_highpass(&analyzer->high_pass, crossover_high, analyzer->sample_rate);
}
