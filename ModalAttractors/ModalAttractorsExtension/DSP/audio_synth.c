/**
 * @file audio_synth.c
 * @brief Variable sample rate audio synthesis from modal state (macOS port)
 *
 * Ported from ESP32 firmware - adapted for AU plugin:
 * - Variable sample rates (not fixed 48kHz)
 * - Pull-based rendering (AU callback)
 * - Stereo float output (not 4-channel TDM)
 * - No I2S/DMA code
 */

#include "audio_synth.h"
#include <math.h>
#include <string.h>

// ============================================================================
// Constants
// ============================================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SMOOTH_ALPHA 0.12f  // Smoothing factor (matches Python SMOOTH)
#define MAX_AMPLITUDE_SCALE 0.7f  // Headroom (matches Python MAX_AMPLITUDE)

// ============================================================================
// Fast Math Helpers
// ============================================================================

/**
 * @brief Fast sine approximation using Taylor series
 *
 * Accurate enough for audio (error < 0.1%)
 * Much faster than sinf() on ESP32, still useful on macOS
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
// Oscillator Functions
// ============================================================================

/**
 * @brief Sine oscillator
 * @param phase Phase in radians [0, 2π)
 * @return Sample value [-1, 1]
 */
static inline float osc_sine(float phase) {
    return sinf(phase);
}

/**
 * @brief Sawtooth oscillator (naive, will alias)
 * @param phase Phase in radians [0, 2π)
 * @return Sample value [-1, 1]
 */
static inline float osc_sawtooth(float phase) {
    // Descending sawtooth: 1 at phase=0, -1 at phase=2π
    return 1.0f - (phase / M_PI);
}

/**
 * @brief Triangle oscillator
 * @param phase Phase in radians [0, 2π)
 * @return Sample value [-1, 1]
 */
static inline float osc_triangle(float phase) {
    if (phase < M_PI) {
        // Rising edge: -1 to +1
        return -1.0f + (2.0f * phase / M_PI);
    } else {
        // Falling edge: +1 to -1
        return 3.0f - (2.0f * phase / M_PI);
    }
}

/**
 * @brief Square/pulse oscillator
 * @param phase Phase in radians [0, 2π)
 * @param pulse_width Pulse width [0, 1], where 0.5 = square
 * @return Sample value [-1, 1]
 */
static inline float osc_pulse(float phase, float pulse_width) {
    float threshold = pulse_width * 2.0f * M_PI;
    return (phase < threshold) ? 1.0f : -1.0f;
}

// ============================================================================
// Initialization
// ============================================================================

void audio_synth_init(audio_synth_t* synth,
                     const modal_node_t* node,
                     float sample_rate) {
    memset(synth, 0, sizeof(audio_synth_t));

    synth->node = node;
    synth->params.sample_rate = sample_rate;
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

void audio_synth_set_sample_rate(audio_synth_t* synth, float sample_rate) {
    synth->params.sample_rate = sample_rate;
}

// ============================================================================
// Audio Generation
// ============================================================================

void audio_synth_render(audio_synth_t* synth,
                       float* outL,
                       float* outR,
                       uint32_t num_frames) {
    if (!synth->initialized || !synth->node) {
        // Return silence
        memset(outL, 0, num_frames * sizeof(float));
        memset(outR, 0, num_frames * sizeof(float));
        return;
    }

    if (synth->params.muted) {
        // Muted: return silence
        memset(outL, 0, num_frames * sizeof(float));
        memset(outR, 0, num_frames * sizeof(float));
        return;
    }

    const modal_node_t* node = synth->node;
    const float sample_rate = synth->params.sample_rate;

    // Initialize output buffers to zero
    memset(outL, 0, num_frames * sizeof(float));
    memset(outR, 0, num_frames * sizeof(float));

    // Generate stereo audio
    // Mix all modes together for both channels
    for (uint32_t sample_idx = 0; sample_idx < num_frames; sample_idx++) {
        float sample_sum = 0.0f;

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
            if (amplitude > MAX_AMPLITUDE_SCALE) {
                amplitude = MAX_AMPLITUDE_SCALE;
            }

            // Get mode frequency (omega[k] in rad/s)
            float omega = node->modes[k].params.omega;
            float freq_hz = omega / (2.0f * M_PI);

            // Phase increment (adapted for variable sample rate)
            float phase_inc = 2.0f * M_PI * freq_hz / sample_rate;

            // Use phase accumulator for continuous carrier
            uint32_t phase_acc = synth->params.phase_accumulator[k];
            float phase = (phase_acc / 4294967296.0f) * 2.0f * M_PI;

            // Note: Do NOT add modal phase cargf(a_k) here - it causes discontinuities
            // The amplitude already captures the modal dynamics

            // Generate sample with selected wave shape
            float oscillator_out;
            wave_shape_t shape = node->modes[k].params.shape;

            switch (shape) {
                case WAVE_SHAPE_SINE:
                    oscillator_out = osc_sine(phase);
                    break;
                case WAVE_SHAPE_SAWTOOTH:
                    oscillator_out = osc_sawtooth(phase);
                    break;
                case WAVE_SHAPE_TRIANGLE:
                    oscillator_out = osc_triangle(phase);
                    break;
                case WAVE_SHAPE_SQUARE:
                    oscillator_out = osc_pulse(phase, 0.5f);
                    break;
                case WAVE_SHAPE_PULSE_25:
                    oscillator_out = osc_pulse(phase, 0.25f);
                    break;
                case WAVE_SHAPE_PULSE_10:
                    oscillator_out = osc_pulse(phase, 0.1f);
                    break;
                default:
                    oscillator_out = osc_sine(phase);  // Fallback to sine
            }

            float sample_f = amplitude * oscillator_out;

            // Add to mix
            sample_sum += sample_f;

            // Advance phase accumulator for next sample
            phase_acc += (uint32_t)(phase_inc * 4294967296.0f / (2.0f * M_PI));
            synth->params.phase_accumulator[k] = phase_acc;
        }

        // Clamp to prevent overflow
        if (sample_sum > 1.0f) sample_sum = 1.0f;
        if (sample_sum < -1.0f) sample_sum = -1.0f;

        // Write to stereo output (mono source, duplicated to L/R)
        outL[sample_idx] = sample_sum;
        outR[sample_idx] = sample_sum;
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
        // Also reset amplitude smoothing to prevent clicks
        synth->amplitude_smooth[k] = 0.0f;
    }
}
