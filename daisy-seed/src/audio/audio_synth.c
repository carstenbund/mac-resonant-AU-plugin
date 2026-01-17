/**
 * @file audio_synth.c
 * @brief Audio synthesis for Daisy Seed (adapted from ESP32)
 *
 * Generates stereo audio from 4 modal oscillators.
 * Each mode contributes to both L/R channels with spatial panning.
 */

#include "audio_synth.h"
#include <math.h>
#include <string.h>
#include <complex.h>

// ============================================================================
// Constants
// ============================================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SMOOTH_ALPHA 0.12f  // Smoothing factor
#define MAX_AMPLITUDE_SCALE 0.5f  // Headroom for mixing 4 modes

// ============================================================================
// Fast Math Helpers
// ============================================================================

/**
 * @brief Fast sine approximation using Taylor series
 */
float fast_sin(float x) {
    // Normalize to [-π, π]
    while (x > M_PI) x -= 2.0f * M_PI;
    while (x < -M_PI) x += 2.0f * M_PI;

    // Taylor series: sin(x) ≈ x - x³/6 + x⁵/120
    float x2 = x * x;
    float x3 = x * x2;
    float x5 = x3 * x2;

    return x - (x3 / 6.0f) + (x5 / 120.0f);
}

/**
 * @brief Hann window envelope
 */
float envelope_hann(float t) {
    if (t < 0.0f) return 0.0f;
    if (t > 1.0f) return 0.0f;
    return 0.5f * (1.0f - cosf(M_PI * t));
}

// ============================================================================
// Initialization
// ============================================================================

void audio_synth_init(audio_synth_t* synth,
                     const modal_node_t* node) {
    memset(synth, 0, sizeof(audio_synth_t));

    synth->node = node;
    synth->params.sample_rate = SAMPLE_RATE;
    synth->params.master_gain = 1.0f;
    synth->params.muted = false;

    // Initialize per-mode parameters
    for (int k = 0; k < MAX_MODES; k++) {
        synth->params.phase_accumulator[k] = 0;
        synth->params.mode_gains[k] = 1.0f;
        synth->amplitude_smooth[k] = 0.0f;
    }

    synth->initialized = true;
}

// ============================================================================
// Audio Generation (Daisy Callback Compatible)
// ============================================================================

void audio_synth_process(audio_synth_t* synth,
                        float* left,
                        float* right,
                        size_t size) {
    if (!synth->initialized || !synth->node) {
        // Return silence
        memset(left, 0, size * sizeof(float));
        memset(right, 0, size * sizeof(float));
        return;
    }

    if (synth->params.muted) {
        // Muted: return silence
        memset(left, 0, size * sizeof(float));
        memset(right, 0, size * sizeof(float));
        return;
    }

    const modal_node_t* node = synth->node;

    // Generate stereo audio
    for (size_t sample_idx = 0; sample_idx < size; sample_idx++) {
        float left_sample = 0.0f;
        float right_sample = 0.0f;

        // Mix all active modes
        for (int k = 0; k < MAX_MODES; k++) {
            // Skip inactive modes
            if (!node->modes[k].params.active) {
                continue;
            }

            // Get mode amplitude (|a_k|)
            float amplitude_raw = cabsf(node->modes[k].a);

            // Apply mode weight
            amplitude_raw *= node->modes[k].params.weight;

            // Smooth amplitude to avoid clicks
            synth->amplitude_smooth[k] +=
                SMOOTH_ALPHA * (amplitude_raw - synth->amplitude_smooth[k]);

            // Final amplitude with gains
            float amplitude = synth->amplitude_smooth[k] *
                            synth->params.mode_gains[k] *
                            synth->params.master_gain *
                            MAX_AMPLITUDE_SCALE;

            // Clip to safe range
            if (amplitude > 1.0f) {
                amplitude = 1.0f;
            }

            // Get mode frequency (omega[k] in rad/s)
            float omega = node->modes[k].params.omega;
            float freq_hz = omega / (2.0f * M_PI);

            // Phase increment
            float phase_inc = 2.0f * M_PI * freq_hz / synth->params.sample_rate;

            // Get phase from accumulator
            uint32_t phase_acc = synth->params.phase_accumulator[k];
            float phase = (phase_acc / 4294967296.0f) * 2.0f * M_PI;

            // Add phase from complex amplitude (for coherence)
            float mode_phase = cargf(node->modes[k].a);
            phase += mode_phase;

            // Generate sample with fast sine
            float sample = amplitude * fast_sin(phase);

            // Stereo panning based on mode index
            // Mode 0,1 → left bias, Mode 2,3 → right bias
            float pan = 0.5f;  // Center by default

            if (k == 0) pan = 0.3f;      // Left
            else if (k == 1) pan = 0.4f; // Left-center
            else if (k == 2) pan = 0.6f; // Right-center
            else if (k == 3) pan = 0.7f; // Right

            // Mix with panning
            left_sample += sample * (1.0f - pan);
            right_sample += sample * pan;

            // Advance phase accumulator (only once per sample, after processing)
            if (sample_idx == size - 1) {
                phase_acc += (uint32_t)(phase_inc * 4294967296.0f / (2.0f * M_PI));
                synth->params.phase_accumulator[k] = phase_acc;
            }
        }

        // Write to output buffers
        left[sample_idx] = left_sample;
        right[sample_idx] = right_sample;
    }

    // Advance all phase accumulators
    for (int k = 0; k < MAX_MODES; k++) {
        if (!node->modes[k].params.active) continue;

        float omega = node->modes[k].params.omega;
        float freq_hz = omega / (2.0f * M_PI);
        float phase_inc = 2.0f * M_PI * freq_hz / synth->params.sample_rate;

        uint32_t phase_acc = synth->params.phase_accumulator[k];
        phase_acc += (uint32_t)(phase_inc * size * 4294967296.0f / (2.0f * M_PI));
        synth->params.phase_accumulator[k] = phase_acc;
    }
}

// ============================================================================
// Control Functions
// ============================================================================

void audio_synth_set_mode_gain(audio_synth_t* synth, int mode_idx, float gain) {
    if (mode_idx < 0 || mode_idx >= MAX_MODES) return;

    if (gain < 0.0f) gain = 0.0f;
    if (gain > 1.0f) gain = 1.0f;

    synth->params.mode_gains[mode_idx] = gain;
}

void audio_synth_set_gain(audio_synth_t* synth, float gain) {
    synth->params.master_gain = (gain < 0.0f) ? 0.0f : (gain > 1.0f) ? 1.0f : gain;
}

void audio_synth_set_mute(audio_synth_t* synth, bool mute) {
    synth->params.muted = mute;
}

void audio_synth_reset_phase(audio_synth_t* synth) {
    for (int k = 0; k < MAX_MODES; k++) {
        synth->params.phase_accumulator[k] = 0;
    }
}
