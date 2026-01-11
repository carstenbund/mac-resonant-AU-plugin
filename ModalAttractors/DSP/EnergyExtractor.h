//
//  EnergyExtractor.h
//  ModalAttractors
//
//  Energy extraction from audio input for resonant body excitation.
//  Computes RMS amplitude with attack/release envelope smoothing.
//
//  Created by Claude on 2026-01-11.
//

#ifndef EnergyExtractor_h
#define EnergyExtractor_h

#include <cstdint>
#include <cmath>
#include <algorithm>

/**
 * @brief Extracts broadband energy from audio input with envelope smoothing
 *
 * This class computes RMS (Root Mean Square) amplitude from incoming audio
 * and applies attack/release envelope following. The output represents how
 * hard the input audio is "exciting" the resonant body.
 *
 * Mental model: This is like measuring how hard you strike a drum or pluck a string.
 */
class EnergyExtractor {
public:
    EnergyExtractor();
    ~EnergyExtractor() = default;

    /**
     * @brief Initialize the extractor with sample rate
     * @param sample_rate Audio sample rate in Hz (e.g., 48000.0)
     */
    void initialize(float sample_rate);

    /**
     * @brief Process a block of audio and extract energy
     * @param input Pointer to input audio samples (mono or use left channel)
     * @param num_frames Number of samples in the block
     * @return Smoothed energy level (0.0 to 1.0+)
     *
     * This method can be called multiple times per control update.
     * It accumulates RMS internally and applies envelope smoothing.
     */
    float process(const float* input, uint32_t num_frames);

    /**
     * @brief Set attack time (how fast energy responds to increases)
     * @param attack_ms Attack time in milliseconds (default: 5ms)
     *
     * Fast attack (~5ms) allows transients to pass through.
     * Slow attack (~50ms) smooths out rapid variations.
     */
    void setAttackTime(float attack_ms);

    /**
     * @brief Set release time (how fast energy decays)
     * @param release_ms Release time in milliseconds (default: 100ms)
     *
     * Fast release (~20ms) makes resonator respond quickly to silence.
     * Slow release (~200ms) maintains excitation during brief gaps.
     */
    void setReleaseTime(float release_ms);

    /**
     * @brief Get current smoothed energy level
     * @return Energy level (0.0 to 1.0+, where 1.0 = full-scale sine wave)
     */
    float getEnergy() const { return envelope_; }

    /**
     * @brief Reset internal state
     */
    void reset();

private:
    float sample_rate_;       ///< Audio sample rate (Hz)
    float envelope_;          ///< Current envelope level (smoothed RMS)

    float attack_coeff_;      ///< Attack coefficient for exponential smoothing
    float release_coeff_;     ///< Release coefficient for exponential smoothing

    float attack_time_ms_;    ///< Attack time in milliseconds
    float release_time_ms_;   ///< Release time in milliseconds

    /**
     * @brief Compute attack/release coefficients from time constants
     *
     * Uses exponential envelope: y[n] = y[n-1] + coeff * (target - y[n-1])
     * Coefficient computed as: 1 - exp(-1 / (time_ms * sample_rate / 1000))
     */
    void updateCoefficients();

    /**
     * @brief Compute RMS amplitude from audio block
     * @param input Audio samples
     * @param num_frames Number of samples
     * @return RMS amplitude (0.0 to 1.0+ for normalized audio)
     */
    float computeRMS(const float* input, uint32_t num_frames);
};

#endif /* EnergyExtractor_h */
