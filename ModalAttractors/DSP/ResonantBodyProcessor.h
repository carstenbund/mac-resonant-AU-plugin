//
//  ResonantBodyProcessor.h
//  ModalAttractors
//
//  Resonant body effect processor - injects audio energy into modal resonators.
//  This is NOT a pitch tracker or filter - it's a stateful physical resonator.
//
//  Created by Claude on 2026-01-11.
//

#ifndef ResonantBodyProcessor_h
#define ResonantBodyProcessor_h

#include "modal_node.h"
#include "audio_synth.h"
#include "EnergyExtractor.h"
#include "SpectralAnalyzer.h"
#include "PitchDetector.h"
#include <cstdint>

/**
 * @brief Resonant body effect processor
 *
 * Mental model: This is like a physical object bolted onto the audio path:
 *   - Sympathetic strings on a guitar
 *   - A piano soundboard reacting to a violin
 *   - A metal plate attached to a drum
 *   - A spring reverb before the reverb exists
 *
 * The input audio INJECTS ENERGY into the resonator.
 * The resonator has its own:
 *   - Modal structure (frequencies, dampings)
 *   - Inertia (keeps ringing after input stops)
 *   - Pitch tendencies (can morph slowly toward input)
 *
 * Signal flow:
 *   Input → Energy Extraction → Spectral Analysis → Modal Excitation →
 *   → Resonator Response → Mix with Dry → Output
 *
 * This is NOT:
 *   - A resonant filter (linear, memoryless)
 *   - A pitch follower (doesn't track pitch exactly)
 *   - A vocoder (doesn't impose spectral envelope)
 *
 * This IS:
 *   - A nonlinear, stateful resonator effect
 *   - A physical body that stores energy
 *   - An autonomous system with its own behavior
 */
class ResonantBodyProcessor {
public:
    ResonantBodyProcessor();
    ~ResonantBodyProcessor();

    /**
     * @brief Initialize processor with sample rate
     * @param sample_rate Audio sample rate in Hz
     */
    void initialize(float sample_rate);

    /**
     * @brief Process audio block (main processing function)
     * @param inputL Left channel input
     * @param inputR Right channel input (currently averaged with left)
     * @param outputL Left channel output
     * @param outputR Right channel output
     * @param num_frames Number of samples to process
     *
     * This performs the full signal chain:
     * 1. Extract energy and spectral content from input
     * 2. Inject energy into modal resonators
     * 3. Update resonator physics
     * 4. Render resonator audio
     * 5. Mix with dry signal
     */
    void process(const float* inputL, const float* inputR,
                 float* outputL, float* outputR,
                 uint32_t num_frames);

    // ========================================================================
    // Effect-style user parameters
    // ========================================================================

    /**
     * @brief Set body size (scales all resonator frequencies)
     * @param size Body size (0.0 to 1.0)
     *   - 0.0 = very small (high pitched, 4x frequency)
     *   - 0.5 = normal (1x frequency)
     *   - 1.0 = very large (low pitched, 0.25x frequency)
     *
     * Maps to frequency scale: size → [4.0, 0.25] (inverse)
     */
    void setBodySize(float size);

    /**
     * @brief Set material type (controls damping distribution)
     * @param material Material hardness (0.0 to 1.0)
     *   - 0.0 = soft, lossy (high damping, short decay)
     *   - 0.5 = balanced (moderate decay)
     *   - 1.0 = hard, resonant (low damping, long ring)
     *
     * Maps to damping: material → damping coefficient
     */
    void setMaterial(float material);

    /**
     * @brief Set excitation amount (how much input drives resonator)
     * @param excite Excitation level (0.0 to 1.0)
     *   - 0.0 = no excitation (resonator silent)
     *   - 0.5 = moderate excitation
     *   - 1.0 = maximum excitation (strong coupling to input)
     *
     * Maps to energy injection gain
     */
    void setExcitation(float excite);

    /**
     * @brief Set pitch morph amount (how flexible body is to input pitch)
     * @param morph Morph amount (0.0 to 1.0)
     *   - 0.0 = fixed tuning (resonator never changes pitch)
     *   - 0.5 = gentle morphing (slow drift toward input)
     *   - 1.0 = maximum morphing (tracks input pitch more closely)
     *
     * This is NOT pitch tracking - it's slow, gentle bias.
     * Think: temperature changes in wood, humidity affecting strings.
     */
    void setMorph(float morph);

    /**
     * @brief Set dry/wet mix
     * @param mix Mix amount (0.0 to 1.0)
     *   - 0.0 = 100% dry (input passes through unaffected)
     *   - 0.5 = 50/50 mix
     *   - 1.0 = 100% wet (only resonator output)
     */
    void setMix(float mix);

    /**
     * @brief Set base frequency for resonator modes
     * @param freq_hz Base frequency in Hz (e.g., 440.0 for A4)
     *
     * This sets the fundamental frequency. Mode frequencies will be
     * harmonically or inharmonically related based on mode multipliers.
     */
    void setBaseFrequency(float freq_hz);

    /**
     * @brief Get current resonator energy level
     * @return Energy (0.0 to 1.0+) - useful for visual feedback
     */
    float getResonatorEnergy() const;

    /**
     * @brief Reset all state (silence resonators, clear buffers)
     */
    void reset();

private:
    // Core resonator components
    modal_node_t modal_node_;       ///< Modal resonator (4 complex modes)
    audio_synth_t audio_synth_;     ///< Audio synthesis from modes

    // Input analysis components
    EnergyExtractor energy_extractor_;     ///< Broadband energy tracker
    SpectralAnalyzer spectral_analyzer_;   ///< 3-band spectral analysis
    PitchDetector pitch_detector_;         ///< Pitch detection for morphing

    // Parameters
    float sample_rate_;
    float body_size_;          ///< 0-1: Body size (frequency scale)
    float material_;           ///< 0-1: Material hardness (damping)
    float excitation_;         ///< 0-1: Excitation amount
    float morph_;              ///< 0-1: Pitch morph amount
    float mix_;                ///< 0-1: Dry/wet mix

    float base_frequency_;     ///< Base frequency in Hz
    float frequency_scale_;    ///< Current frequency scale from body size

    // Mode configuration (default harmonic series)
    float mode_freq_multipliers_[MAX_MODES];  ///< Mode frequency ratios

    // Control rate tracking
    uint32_t samples_until_control_update_;
    uint32_t control_period_samples_;

    /**
     * @brief Update resonator parameters based on effect parameters
     *
     * Called periodically (not every sample) to update modal frequencies,
     * dampings, etc. based on current parameter values.
     */
    void updateResonatorParameters();

    /**
     * @brief Apply pitch morphing (slow frequency drift toward input)
     * @param detected_pitch Detected pitch in Hz (0 if none)
     * @param confidence Confidence of pitch detection (0-1)
     *
     * Only called when morph > 0 and pitch detection is confident.
     * Updates base_frequency_ slowly toward detected pitch.
     */
    void applyPitchMorph(float detected_pitch, float confidence);

    /**
     * @brief Inject energy into resonator based on analysis
     * @param energy Broadband energy level
     * @param band_energies[3] Spectral band energies
     *
     * Creates a poke event and applies it to the modal node.
     * Band energies weight the excitation per mode.
     */
    void injectEnergy(float energy, const float band_energies[3]);

    /**
     * @brief Map body size to frequency scale
     * @param size Body size parameter (0-1)
     * @return Frequency scale (0.25 - 4.0)
     */
    float mapBodySizeToFrequencyScale(float size) const;

    /**
     * @brief Map material to damping coefficient
     * @param material Material parameter (0-1)
     * @return Damping coefficient (0.1 - 5.0)
     */
    float mapMaterialToDamping(float material) const;
};

#endif /* ResonantBodyProcessor_h */
