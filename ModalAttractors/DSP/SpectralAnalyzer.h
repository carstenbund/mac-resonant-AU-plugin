//
//  SpectralAnalyzer.h
//  ModalAttractors
//
//  Multi-band spectral analysis for resonant body excitation weighting.
//  Splits input into frequency bands to determine which modes to excite.
//
//  Created by Claude on 2026-01-11.
//

#ifndef SpectralAnalyzer_h
#define SpectralAnalyzer_h

#include <cstdint>
#include <cmath>

/**
 * @brief Simple biquad filter for bandpass/lowpass/highpass filtering
 *
 * Direct Form II transposed biquad filter.
 * Used internally by SpectralAnalyzer for band separation.
 */
class BiquadFilter {
public:
    BiquadFilter() : b0(1), b1(0), b2(0), a1(0), a2(0), z1(0), z2(0) {}

    /**
     * @brief Configure as lowpass filter
     * @param sample_rate Sample rate in Hz
     * @param cutoff_freq Cutoff frequency in Hz
     * @param q Q factor (default 0.707 = Butterworth)
     */
    void setLowpass(float sample_rate, float cutoff_freq, float q = 0.707f);

    /**
     * @brief Configure as highpass filter
     * @param sample_rate Sample rate in Hz
     * @param cutoff_freq Cutoff frequency in Hz
     * @param q Q factor (default 0.707 = Butterworth)
     */
    void setHighpass(float sample_rate, float cutoff_freq, float q = 0.707f);

    /**
     * @brief Configure as bandpass filter
     * @param sample_rate Sample rate in Hz
     * @param center_freq Center frequency in Hz
     * @param bandwidth Bandwidth in Hz
     */
    void setBandpass(float sample_rate, float center_freq, float bandwidth);

    /**
     * @brief Process a single sample
     * @param input Input sample
     * @return Filtered output sample
     */
    inline float process(float input) {
        float output = b0 * input + z1;
        z1 = b1 * input - a1 * output + z2;
        z2 = b2 * input - a2 * output;
        return output;
    }

    /**
     * @brief Reset filter state
     */
    void reset() {
        z1 = z2 = 0.0f;
    }

private:
    // Biquad coefficients
    float b0, b1, b2;  ///< Feedforward coefficients
    float a1, a2;      ///< Feedback coefficients (a0 = 1 implicit)

    // State variables (Direct Form II Transposed)
    float z1, z2;
};

/**
 * @brief Multi-band spectral analyzer for modal excitation weighting
 *
 * Splits input audio into 3 frequency bands and computes RMS per band.
 * This enables spectral shaping of modal resonator excitation:
 *   - Low band energy excites low-frequency modes
 *   - Mid band energy excites mid-frequency modes
 *   - High band energy excites high-frequency modes
 *
 * Band definitions:
 *   - Low:  20 Hz - 400 Hz  (bass, fundamentals)
 *   - Mid:  400 Hz - 3 kHz  (body, harmonics)
 *   - High: 3 kHz - 20 kHz  (brightness, transients)
 */
class SpectralAnalyzer {
public:
    /**
     * @brief Number of frequency bands
     */
    static constexpr uint32_t NUM_BANDS = 3;

    /**
     * @brief Frequency band indices
     */
    enum Band {
        LOW = 0,   ///< 20 Hz - 400 Hz
        MID = 1,   ///< 400 Hz - 3 kHz
        HIGH = 2   ///< 3 kHz - 20 kHz
    };

    SpectralAnalyzer();
    ~SpectralAnalyzer() = default;

    /**
     * @brief Initialize the analyzer with sample rate
     * @param sample_rate Audio sample rate in Hz
     */
    void initialize(float sample_rate);

    /**
     * @brief Process a block of audio and extract band energies
     * @param input Input audio samples
     * @param num_frames Number of samples in the block
     * @param band_energies Output array for band RMS values [3]
     *
     * Fills band_energies[LOW], band_energies[MID], band_energies[HIGH]
     * with RMS amplitude per band (0.0 to 1.0+).
     */
    void process(const float* input, uint32_t num_frames, float band_energies[NUM_BANDS]);

    /**
     * @brief Get the most recent band energies
     * @param band_energies Output array for band RMS values [3]
     */
    void getBandEnergies(float band_energies[NUM_BANDS]) const;

    /**
     * @brief Set smoothing time for band energy tracking
     * @param smoothing_ms Smoothing time in milliseconds (default: 20ms)
     *
     * Larger values = smoother, more averaged band energies
     * Smaller values = faster response to spectral changes
     */
    void setSmoothingTime(float smoothing_ms);

    /**
     * @brief Reset all filters and state
     */
    void reset();

private:
    float sample_rate_;

    // Filter bank for band separation
    BiquadFilter low_filter_;     ///< Lowpass @ 400 Hz
    BiquadFilter mid_filter_low_; ///< Highpass @ 400 Hz (mid band lower edge)
    BiquadFilter mid_filter_high_;///< Lowpass @ 3 kHz (mid band upper edge)
    BiquadFilter high_filter_;    ///< Highpass @ 3 kHz

    // Band energy state (smoothed RMS per band)
    float band_rms_[NUM_BANDS];

    // Smoothing coefficients
    float smoothing_coeff_;
    float smoothing_time_ms_;

    /**
     * @brief Compute RMS of audio block
     * @param input Audio samples
     * @param num_frames Number of samples
     * @return RMS amplitude
     */
    float computeRMS(const float* input, uint32_t num_frames);

    /**
     * @brief Update smoothing coefficient from time constant
     */
    void updateSmoothingCoeff();
};

#endif /* SpectralAnalyzer_h */
