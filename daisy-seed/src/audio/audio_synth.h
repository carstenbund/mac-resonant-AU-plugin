/**
 * @file audio_synth.h
 * @brief Audio synthesis for Daisy Seed (adapted from ESP32)
 *
 * Daisy-specific changes:
 * - Callback-based instead of buffer-push
 * - Stereo output instead of 4-channel
 * - Compatible with libDaisy AudioHandle
 *
 * Channel mapping options:
 * - Stereo sum: All modes mixed to L/R
 * - Stereo split: Modes 0,1 → L, Modes 2,3 → R
 * - Spatial: Pan based on mode phase
 */

#ifndef AUDIO_SYNTH_H
#define AUDIO_SYNTH_H

#include <stdint.h>
#include <stdbool.h>
#include "../core/modal_node.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Constants
// ============================================================================

#define SAMPLE_RATE 48000
#define MAX_BLOCK_SIZE 256  // Daisy typically uses 48-256 samples

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief Audio synthesis parameters
 */
typedef struct {
    float sample_rate;                    ///< Sample rate (Hz)
    uint32_t phase_accumulator[MAX_MODES]; ///< Phase accumulators (one per mode)
    float mode_gains[MAX_MODES];          ///< Per-mode gains [0,1]
    float master_gain;                    ///< Master output gain [0,1]
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
 */
void audio_synth_init(audio_synth_t* synth,
                     const modal_node_t* node);

/**
 * @brief Generate stereo audio samples (callback-style)
 *
 * Fills the provided stereo buffers with audio samples.
 * Called by Daisy AudioCallback.
 *
 * @param synth Pointer to synthesis state
 * @param left Left channel output buffer
 * @param right Right channel output buffer
 * @param size Number of samples to generate
 */
void audio_synth_process(audio_synth_t* synth,
                        float* left,
                        float* right,
                        size_t size);

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

// ============================================================================
// Synthesis Helpers
// ============================================================================

/**
 * @brief Fast sine approximation
 *
 * Uses Taylor series for fast computation.
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
