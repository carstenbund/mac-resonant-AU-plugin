/**
 * @file SpectralAnalyzer.h
 * @brief 3-band biquad filter bank for spectral analysis
 *
 * Splits audio signal into 3 frequency bands (Low, Mid, High) using
 * biquad filters for resonator excitation.
 */

#ifndef SPECTRAL_ANALYZER_H
#define SPECTRAL_ANALYZER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Frequency band identifiers
 */
typedef enum {
    BAND_LOW = 0,   ///< Low frequency band
    BAND_MID = 1,   ///< Mid frequency band
    BAND_HIGH = 2,  ///< High frequency band
    NUM_BANDS = 3   ///< Total number of bands
} spectral_band_t;

/**
 * @brief Biquad filter state
 */
typedef struct {
    // Filter coefficients
    float a0, a1, a2;  ///< Feedback coefficients
    float b0, b1, b2;  ///< Feedforward coefficients

    // Filter state
    float x1, x2;  ///< Input history
    float y1, y2;  ///< Output history
} biquad_t;

/**
 * @brief Spectral analyzer state (3-band filter bank)
 */
typedef struct {
    float sample_rate;        ///< Sample rate in Hz
    biquad_t low_pass;       ///< Low-pass filter for low band
    biquad_t band_pass;      ///< Band-pass filter for mid band
    biquad_t high_pass;      ///< High-pass filter for high band
    float crossover_low;     ///< Low/mid crossover frequency (Hz)
    float crossover_high;    ///< Mid/high crossover frequency (Hz)
    bool initialized;        ///< Initialization flag
} spectral_analyzer_t;

/**
 * @brief Initialize spectral analyzer
 *
 * @param analyzer Pointer to analyzer structure
 * @param sample_rate Sample rate in Hz
 * @param crossover_low Low/mid crossover frequency (default: 300 Hz)
 * @param crossover_high Mid/high crossover frequency (default: 3000 Hz)
 */
void spectral_analyzer_init(spectral_analyzer_t* analyzer,
                           float sample_rate,
                           float crossover_low,
                           float crossover_high);

/**
 * @brief Process a single sample through all bands
 *
 * @param analyzer Pointer to analyzer structure
 * @param input Input sample
 * @param band_outputs Output array for 3 bands [low, mid, high]
 */
void spectral_analyzer_process(spectral_analyzer_t* analyzer,
                              float input,
                              float band_outputs[NUM_BANDS]);

/**
 * @brief Process a buffer of samples through all bands
 *
 * @param analyzer Pointer to analyzer structure
 * @param input Input buffer
 * @param low_output Low band output buffer
 * @param mid_output Mid band output buffer
 * @param high_output High band output buffer
 * @param num_samples Number of samples to process
 */
void spectral_analyzer_process_buffer(spectral_analyzer_t* analyzer,
                                      const float* input,
                                      float* low_output,
                                      float* mid_output,
                                      float* high_output,
                                      uint32_t num_samples);

/**
 * @brief Reset all filter states
 *
 * @param analyzer Pointer to analyzer structure
 */
void spectral_analyzer_reset(spectral_analyzer_t* analyzer);

/**
 * @brief Update crossover frequencies
 *
 * @param analyzer Pointer to analyzer structure
 * @param crossover_low Low/mid crossover frequency (Hz)
 * @param crossover_high Mid/high crossover frequency (Hz)
 */
void spectral_analyzer_set_crossovers(spectral_analyzer_t* analyzer,
                                     float crossover_low,
                                     float crossover_high);

#ifdef __cplusplus
}
#endif

#endif // SPECTRAL_ANALYZER_H
