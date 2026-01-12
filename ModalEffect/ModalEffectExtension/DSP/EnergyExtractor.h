/**
 * @file EnergyExtractor.h
 * @brief RMS energy analysis with attack/release envelope
 *
 * Extracts the energy envelope from an audio signal using RMS analysis
 * with separate attack and release time constants.
 */

#ifndef ENERGY_EXTRACTOR_H
#define ENERGY_EXTRACTOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Energy extractor state
 */
typedef struct {
    float sample_rate;      ///< Sample rate in Hz
    float envelope;         ///< Current envelope value [0, 1]
    float attack_coeff;     ///< Attack time coefficient
    float release_coeff;    ///< Release time coefficient
    float rms_window_sum;   ///< Running sum for RMS calculation
    uint32_t rms_window_size; ///< RMS window size in samples
    float* rms_buffer;      ///< Circular buffer for RMS calculation
    uint32_t rms_index;     ///< Current index in RMS buffer
    bool initialized;       ///< Initialization flag
} energy_extractor_t;

/**
 * @brief Initialize energy extractor
 *
 * @param extractor Pointer to extractor structure
 * @param sample_rate Sample rate in Hz
 * @param attack_ms Attack time in milliseconds (default: 5ms)
 * @param release_ms Release time in milliseconds (default: 100ms)
 * @param rms_window_ms RMS window size in milliseconds (default: 10ms)
 */
void energy_extractor_init(energy_extractor_t* extractor,
                          float sample_rate,
                          float attack_ms,
                          float release_ms,
                          float rms_window_ms);

/**
 * @brief Process a single sample
 *
 * @param extractor Pointer to extractor structure
 * @param input Input sample
 * @return Current envelope value [0, 1]
 */
float energy_extractor_process(energy_extractor_t* extractor, float input);

/**
 * @brief Process a buffer of samples
 *
 * @param extractor Pointer to extractor structure
 * @param input Input buffer
 * @param output Envelope output buffer
 * @param num_samples Number of samples to process
 */
void energy_extractor_process_buffer(energy_extractor_t* extractor,
                                     const float* input,
                                     float* output,
                                     uint32_t num_samples);

/**
 * @brief Get current envelope value
 *
 * @param extractor Pointer to extractor structure
 * @return Current envelope value [0, 1]
 */
float energy_extractor_get_envelope(const energy_extractor_t* extractor);

/**
 * @brief Reset the extractor state
 *
 * @param extractor Pointer to extractor structure
 */
void energy_extractor_reset(energy_extractor_t* extractor);

/**
 * @brief Update attack time
 *
 * @param extractor Pointer to extractor structure
 * @param attack_ms Attack time in milliseconds
 */
void energy_extractor_set_attack(energy_extractor_t* extractor, float attack_ms);

/**
 * @brief Update release time
 *
 * @param extractor Pointer to extractor structure
 * @param release_ms Release time in milliseconds
 */
void energy_extractor_set_release(energy_extractor_t* extractor, float release_ms);

/**
 * @brief Clean up and free resources
 *
 * @param extractor Pointer to extractor structure
 */
void energy_extractor_cleanup(energy_extractor_t* extractor);

#ifdef __cplusplus
}
#endif

#endif // ENERGY_EXTRACTOR_H
