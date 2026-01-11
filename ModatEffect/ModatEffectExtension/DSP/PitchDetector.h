/**
 * @file PitchDetector.h
 * @brief Autocorrelation-based pitch detection for morphing
 *
 * Detects fundamental frequency of input signal using autocorrelation
 * with optional smoothing for stable pitch tracking.
 */

#ifndef PITCH_DETECTOR_H
#define PITCH_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pitch detector state
 */
typedef struct {
    float sample_rate;          ///< Sample rate in Hz
    float* buffer;              ///< Input buffer for analysis
    uint32_t buffer_size;       ///< Buffer size in samples
    uint32_t buffer_index;      ///< Current write index
    float min_freq;             ///< Minimum detectable frequency (Hz)
    float max_freq;             ///< Maximum detectable frequency (Hz)
    float detected_pitch;       ///< Current detected pitch (Hz)
    float smoothed_pitch;       ///< Smoothed pitch for morphing (Hz)
    float smoothing_coeff;      ///< Pitch smoothing coefficient
    float confidence;           ///< Detection confidence [0, 1]
    bool initialized;           ///< Initialization flag
    bool pitch_valid;           ///< Valid pitch detected flag
} pitch_detector_t;

/**
 * @brief Initialize pitch detector
 *
 * @param detector Pointer to detector structure
 * @param sample_rate Sample rate in Hz
 * @param min_freq Minimum detectable frequency (default: 60 Hz)
 * @param max_freq Maximum detectable frequency (default: 2000 Hz)
 * @param buffer_size_ms Analysis buffer size in milliseconds (default: 50ms)
 * @param smoothing_ms Pitch smoothing time in milliseconds (default: 100ms)
 */
void pitch_detector_init(pitch_detector_t* detector,
                        float sample_rate,
                        float min_freq,
                        float max_freq,
                        float buffer_size_ms,
                        float smoothing_ms);

/**
 * @brief Process a single sample
 *
 * @param detector Pointer to detector structure
 * @param input Input sample
 */
void pitch_detector_process(pitch_detector_t* detector, float input);

/**
 * @brief Process a buffer of samples
 *
 * @param detector Pointer to detector structure
 * @param input Input buffer
 * @param num_samples Number of samples to process
 */
void pitch_detector_process_buffer(pitch_detector_t* detector,
                                   const float* input,
                                   uint32_t num_samples);

/**
 * @brief Run pitch detection analysis
 *
 * Should be called periodically (e.g., every buffer) after processing samples.
 * This performs the autocorrelation analysis on the buffered data.
 *
 * @param detector Pointer to detector structure
 */
void pitch_detector_analyze(pitch_detector_t* detector);

/**
 * @brief Get detected pitch
 *
 * @param detector Pointer to detector structure
 * @return Detected pitch in Hz (0 if no valid pitch)
 */
float pitch_detector_get_pitch(const pitch_detector_t* detector);

/**
 * @brief Get smoothed pitch for morphing
 *
 * @param detector Pointer to detector structure
 * @return Smoothed pitch in Hz (0 if no valid pitch)
 */
float pitch_detector_get_smoothed_pitch(const pitch_detector_t* detector);

/**
 * @brief Get detection confidence
 *
 * @param detector Pointer to detector structure
 * @return Confidence value [0, 1]
 */
float pitch_detector_get_confidence(const pitch_detector_t* detector);

/**
 * @brief Check if valid pitch is detected
 *
 * @param detector Pointer to detector structure
 * @return True if valid pitch is detected
 */
bool pitch_detector_is_valid(const pitch_detector_t* detector);

/**
 * @brief Reset detector state
 *
 * @param detector Pointer to detector structure
 */
void pitch_detector_reset(pitch_detector_t* detector);

/**
 * @brief Update frequency range
 *
 * @param detector Pointer to detector structure
 * @param min_freq Minimum detectable frequency (Hz)
 * @param max_freq Maximum detectable frequency (Hz)
 */
void pitch_detector_set_range(pitch_detector_t* detector,
                             float min_freq,
                             float max_freq);

/**
 * @brief Clean up and free resources
 *
 * @param detector Pointer to detector structure
 */
void pitch_detector_cleanup(pitch_detector_t* detector);

#ifdef __cplusplus
}
#endif

#endif // PITCH_DETECTOR_H
