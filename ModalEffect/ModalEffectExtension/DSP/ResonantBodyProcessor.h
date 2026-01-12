/**
 * @file ResonantBodyProcessor.h
 * @brief Main resonant body effect processor
 *
 * Integrates energy extraction, spectral analysis, pitch detection,
 * and modal resonators to create a physical-modeled resonant body effect.
 */

#ifndef RESONANT_BODY_PROCESSOR_H
#define RESONANT_BODY_PROCESSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "EnergyExtractor.h"
#include "SpectralAnalyzer.h"
#include "PitchDetector.h"
#include "modal_node.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of resonators (one per frequency band)
#define MAX_RESONATORS 3

/**
 * @brief Effect parameters
 */
typedef struct {
    float body_size;     ///< Body size [0, 1] - scales resonator frequencies
    float material;      ///< Material hardness [0, 1] - controls damping
    float excite;        ///< Excitation amount [0, 1] - input drive
    float morph;         ///< Pitch tracking amount [0, 1] - morphing
    float mix;           ///< Dry/wet mix [0, 1] - effect blend
} resonant_body_params_t;

/**
 * @brief Resonant body processor state
 */
typedef struct {
    float sample_rate;                    ///< Sample rate in Hz

    // DSP components
    energy_extractor_t energy_extractor;  ///< Energy envelope follower
    spectral_analyzer_t spectral_analyzer;///< 3-band filter bank
    pitch_detector_t pitch_detector;      ///< Pitch detection

    // Modal resonators (one per band)
    modal_node_t resonators[MAX_RESONATORS];

    // Parameters
    resonant_body_params_t params;

    // Base frequencies for each band (Hz)
    float base_freqs[MAX_RESONATORS];

    // Control rate counter
    uint32_t control_counter;
    uint32_t control_rate_divisor;

    // State
    bool initialized;
} resonant_body_processor_t;

/**
 * @brief Initialize resonant body processor
 *
 * @param processor Pointer to processor structure
 * @param sample_rate Sample rate in Hz
 */
void resonant_body_init(resonant_body_processor_t* processor, float sample_rate);

/**
 * @brief Process a single sample
 *
 * @param processor Pointer to processor structure
 * @param input Input sample
 * @return Processed output sample
 */
float resonant_body_process(resonant_body_processor_t* processor, float input);

/**
 * @brief Process a buffer of samples (stereo)
 *
 * @param processor Pointer to processor structure
 * @param input_L Left input buffer
 * @param input_R Right input buffer
 * @param output_L Left output buffer
 * @param output_R Right output buffer
 * @param num_samples Number of samples to process
 */
void resonant_body_process_buffer(resonant_body_processor_t* processor,
                                  const float* input_L,
                                  const float* input_R,
                                  float* output_L,
                                  float* output_R,
                                  uint32_t num_samples);

/**
 * @brief Set body size parameter
 *
 * @param processor Pointer to processor structure
 * @param size Body size [0, 1]
 */
void resonant_body_set_body_size(resonant_body_processor_t* processor, float size);

/**
 * @brief Set material parameter
 *
 * @param processor Pointer to processor structure
 * @param material Material hardness [0, 1]
 */
void resonant_body_set_material(resonant_body_processor_t* processor, float material);

/**
 * @brief Set excitation parameter
 *
 * @param processor Pointer to processor structure
 * @param excite Excitation amount [0, 1]
 */
void resonant_body_set_excite(resonant_body_processor_t* processor, float excite);

/**
 * @brief Set morph parameter
 *
 * @param processor Pointer to processor structure
 * @param morph Pitch tracking amount [0, 1]
 */
void resonant_body_set_morph(resonant_body_processor_t* processor, float morph);

/**
 * @brief Set mix parameter
 *
 * @param processor Pointer to processor structure
 * @param mix Dry/wet mix [0, 1]
 */
void resonant_body_set_mix(resonant_body_processor_t* processor, float mix);

/**
 * @brief Reset processor state
 *
 * @param processor Pointer to processor structure
 */
void resonant_body_reset(resonant_body_processor_t* processor);

/**
 * @brief Clean up and free resources
 *
 * @param processor Pointer to processor structure
 */
void resonant_body_cleanup(resonant_body_processor_t* processor);

#ifdef __cplusplus
}
#endif

#endif // RESONANT_BODY_PROCESSOR_H
