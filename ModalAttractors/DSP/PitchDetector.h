//
//  PitchDetector.h
//  ModalAttractors
//
//  Simple pitch detection for resonant body frequency morphing.
//  Uses autocorrelation to find dominant periodicity in input signal.
//
//  Created by Claude on 2026-01-11.
//

#ifndef PitchDetector_h
#define PitchDetector_h

#include <cstdint>
#include <vector>

/**
 * @brief Simple autocorrelation-based pitch detector
 *
 * This detector finds the dominant periodicity in an audio signal using
 * autocorrelation. It's NOT intended for precise pitch tracking (like
 * for a pitch-to-MIDI converter), but rather for gently biasing the
 * resonant body's tuning toward the input's pitch center.
 *
 * Mental model: This detects "where the input wants to resonate" so the
 * resonant body can slowly drift toward that frequency region.
 *
 * Update rate is intentionally slow (~10-50ms) to avoid rapid retuning.
 */
class PitchDetector {
public:
    PitchDetector();
    ~PitchDetector() = default;

    /**
     * @brief Initialize the detector with sample rate
     * @param sample_rate Audio sample rate in Hz
     */
    void initialize(float sample_rate);

    /**
     * @brief Process a block of audio and update pitch estimate
     * @param input Input audio samples (mono)
     * @param num_frames Number of samples in the block
     *
     * This method can be called every audio callback. It accumulates
     * audio internally and updates pitch estimate periodically.
     */
    void process(const float* input, uint32_t num_frames);

    /**
     * @brief Get the current pitch estimate
     * @return Estimated pitch in Hz (or 0.0 if no pitch detected)
     *
     * Returns a slowly-updating pitch estimate. Use this to bias
     * the resonant body's frequency, NOT to directly set it.
     */
    float getPitch() const { return estimated_pitch_hz_; }

    /**
     * @brief Get confidence of current pitch estimate
     * @return Confidence (0.0 to 1.0), where 1.0 = very confident
     *
     * Use this to gate morphing: only apply pitch bias when confidence
     * is above a threshold (e.g., 0.3).
     */
    float getConfidence() const { return confidence_; }

    /**
     * @brief Set analysis window size
     * @param window_ms Analysis window in milliseconds (default: 40ms)
     *
     * Larger window = better low-frequency resolution, slower updates
     * Smaller window = faster response, worse low-frequency resolution
     */
    void setWindowSize(float window_ms);

    /**
     * @brief Set minimum detectable pitch
     * @param min_hz Minimum pitch in Hz (default: 60 Hz, about B1)
     */
    void setMinPitch(float min_hz) { min_pitch_hz_ = min_hz; }

    /**
     * @brief Set maximum detectable pitch
     * @param max_hz Maximum pitch in Hz (default: 2000 Hz, about B6)
     */
    void setMaxPitch(float max_hz) { max_pitch_hz_ = max_hz; }

    /**
     * @brief Reset internal state
     */
    void reset();

private:
    float sample_rate_;

    // Detection range
    float min_pitch_hz_;        ///< Minimum detectable pitch (Hz)
    float max_pitch_hz_;        ///< Maximum detectable pitch (Hz)

    // Analysis window
    float window_size_ms_;      ///< Analysis window size (ms)
    uint32_t window_samples_;   ///< Analysis window in samples

    // Pitch estimate state
    float estimated_pitch_hz_;  ///< Current pitch estimate (Hz)
    float confidence_;          ///< Confidence of estimate (0-1)

    // Audio buffer for analysis
    std::vector<float> audio_buffer_;
    uint32_t buffer_write_pos_;

    /**
     * @brief Run autocorrelation pitch detection on current buffer
     *
     * Finds the lag with maximum autocorrelation within the pitch range.
     * Updates estimated_pitch_hz_ and confidence_.
     */
    void runPitchDetection();

    /**
     * @brief Compute autocorrelation at a given lag
     * @param buffer Audio buffer
     * @param buffer_size Buffer size in samples
     * @param lag Lag in samples
     * @return Autocorrelation value (normalized to buffer energy)
     */
    float computeAutocorrelation(const float* buffer, uint32_t buffer_size, uint32_t lag);
};

#endif /* PitchDetector_h */
