/**
 * @file audio_synth.h
 * @brief Variable sample rate audio synthesis from modal state (macOS port)
 *
 * Ported from ESP32 firmware - adapted for:
 * - Variable sample rates (44.1/48/88.2/96 kHz)
 * - Pull-based rendering (AU callback model)
 * - No I2S/DMA dependencies
 *
 * Each mode synthesizes its own sinusoid at its frequency (omega[k]),
 * with amplitude envelope from the mode's complex amplitude |a_k|.
 */

#ifndef AUDIO_SYNTH_H
#define AUDIO_SYNTH_H

#include <stdint.h>
#include <stdbool.h>
#include "modal_node.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Constants
// ============================================================================

#define DEFAULT_SAMPLE_RATE 48000.0f
#define AUDIO_BUFFER_SAMPLES 512  // Typical AU buffer size
#define NUM_AUDIO_CHANNELS 2      // Stereo output for AU
#define BITS_PER_SAMPLE 32        // Float samples for AU

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief Oscillator wave shapes
 */
typedef enum {
    WAVE_SHAPE_SINE = 0,      ///< Sine wave (pure fundamental)
    WAVE_SHAPE_SAWTOOTH,      ///< Sawtooth (all harmonics, bright)
    WAVE_SHAPE_TRIANGLE,      ///< Triangle (odd harmonics, soft)
    WAVE_SHAPE_SQUARE,        ///< Square wave (odd harmonics, aggressive)
    WAVE_SHAPE_PULSE_25,      ///< Pulse 25% duty cycle
    WAVE_SHAPE_PULSE_10,      ///< Pulse 10% duty cycle (thin)
    WAVE_SHAPE_COUNT          ///< Total number of shapes
} wave_shape_t;

/**
 * @brief Audio synthesis parameters
 */
typedef struct {
    float sample_rate;                    ///< Sample rate (Hz) - variable
    uint32_t phase_accumulator[MAX_MODES]; ///< Phase accumulators (one per mode)
    float mode_gains[MAX_MODES];          ///< Per-mode gains [0,1]
    float master_gain;                    ///< Master output gain [0,1]
    wave_shape_t wave_shape;              ///< Global oscillator wave shape
    float pulse_width;                    ///< Pulse width for pulse shapes [0.01-0.99]
    bool muted;                           ///< Mute flag
} audio_synth_params_t;

/**
 * @brief Audio synthesis state
 */
typedef struct {
    audio_synth_params_t params;
    const modal_node_t* node;           ///< Reference to modal node state
    float amplitude_smooth[MAX_MODES];  ///< Smoothed amplitudes per mode
    bool initialized;
} audio_synth_t;

// ============================================================================
// Core API
// ============================================================================

/**
 * @brief Initialize audio synthesis engine
 *
 * @param synth Pointer to synthesis state
 * @param node Pointer to modal node (state source)
 * @param sample_rate Sample rate in Hz (44100, 48000, 96000, etc.)
 */
void audio_synth_init(audio_synth_t* synth,
                     const modal_node_t* node,
                     float sample_rate);

/**
 * @brief Generate audio samples (stereo float)
 *
 * Reads current modal state and generates audio samples.
 * Called by AU render callback.
 *
 * @param synth Pointer to synthesis state
 * @param outL Left channel output buffer
 * @param outR Right channel output buffer
 * @param num_frames Number of frames to generate
 */
void audio_synth_render(audio_synth_t* synth,
                       float* outL,
                       float* outR,
                       uint32_t num_frames);

/**
 * @brief Set sample rate (for sample rate changes)
 *
 * @param synth Pointer to synthesis state
 * @param sample_rate New sample rate in Hz
 */
void audio_synth_set_sample_rate(audio_synth_t* synth, float sample_rate);

/**
 * @brief Set per-mode gain
 *
 * @param synth Pointer to synthesis state
 * @param mode_idx Mode index [0-3]
 * @param gain Gain [0,1]
 */
void audio_synth_set_mode_gain(audio_synth_t* synth, int mode_idx, float gain);

/**
 * @brief Set master gain
 *
 * @param synth Pointer to synthesis state
 * @param gain Gain [0,1]
 */
void audio_synth_set_gain(audio_synth_t* synth, float gain);

/**
 * @brief Mute/unmute audio
 *
 * @param synth Pointer to synthesis state
 * @param mute Mute flag
 */
void audio_synth_set_mute(audio_synth_t* synth, bool mute);

/**
 * @brief Reset phase (hard sync)
 *
 * @param synth Pointer to synthesis state
 */
void audio_synth_reset_phase(audio_synth_t* synth);

/**
 * @brief Set wave shape
 *
 * @param synth Pointer to synthesis state
 * @param shape Wave shape (WAVE_SHAPE_SINE, etc.)
 */
void audio_synth_set_wave_shape(audio_synth_t* synth, wave_shape_t shape);

/**
 * @brief Set pulse width for pulse wave shapes
 *
 * @param synth Pointer to synthesis state
 * @param width Pulse width [0.01-0.99] (0.5 = square wave)
 */
void audio_synth_set_pulse_width(audio_synth_t* synth, float width);

// ============================================================================
// Synthesis Helpers
// ============================================================================

/**
 * @brief Fast sine approximation
 *
 * Uses Taylor series for low-latency synthesis.
 *
 * @param phase Phase in radians
 * @return Sine value [-1, 1]
 */
float fast_sin(float phase);

/**
 * @brief Apply smooth envelope (for poke transients)
 *
 * @param t Time normalized [0,1]
 * @return Envelope value [0,1]
 */
float envelope_hann(float t);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_SYNTH_H
