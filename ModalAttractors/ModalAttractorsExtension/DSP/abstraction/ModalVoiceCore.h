/**
 * @file ModalVoiceCore.h
 * @brief DSP host wrapper for modal voice control
 *
 * Provides a clean interface for controlling a single modal voice instance
 * that wraps the core DSP (modal_node_t + audio_synth_t). This abstraction
 * allows both MIDI and CV/Gate control through the same API.
 *
 * Part of the DSP abstraction layer architecture.
 */

#pragma once

#include <cstdint>
#include "../modal_node.h"
#include "../audio_synth.h"

/**
 * @brief Excitation parameters for triggering modal resonance
 */
struct ExcitationParams {
    float strength = 0.0f;           ///< Excitation strength [0..1]
    float duration_ms = 8.0f;        ///< Poke duration in milliseconds
    float phase_hint = -1.0f;        ///< Phase hint (radians, -1 = random)
    float mode_weights[MAX_MODES] = {1.0f, 1.0f, 1.0f, 1.0f}; ///< Per-mode weighting
};

/**
 * @brief Modal voice core wrapper
 *
 * Wraps modal_node_t and audio_synth_t with a unified control interface.
 * Manages pitch, damping, mode weights, and excitation parameters.
 */
class ModalVoiceCore {
public:
    /**
     * @brief Construct voice with unique identifier
     * @param voice_id Unique voice/node identifier
     */
    explicit ModalVoiceCore(uint8_t voice_id);

    /**
     * @brief Initialize voice with sample rate
     * @param sample_rate Sample rate in Hz
     */
    void initialize(float sample_rate);

    /**
     * @brief Set pitch in Hz
     * @param hz Fundamental frequency in Hz
     */
    void setPitchHz(float hz);

    /**
     * @brief Set pitch bend offset
     * @param semis Pitch bend in semitones (+/- 12 typical range)
     */
    void setPitchBend(float semis);

    /**
     * @brief Set global damping coefficient
     * @param damping_0to1 Global damping [0..1]
     */
    void setGlobalDamping(float damping_0to1);

    /**
     * @brief Set individual mode damping
     * @param mode Mode index [0..MAX_MODES-1]
     * @param gamma Damping coefficient (>0)
     */
    void setModeDamping(uint8_t mode, float gamma);

    /**
     * @brief Set individual mode weight
     * @param mode Mode index [0..MAX_MODES-1]
     * @param weight Audio contribution weight [0..1]
     */
    void setModeWeight(uint8_t mode, float weight);

    /**
     * @brief Set excitation parameters for next trigger
     * @param params Excitation parameters
     */
    void setExcitationParams(const ExcitationParams& params);

    /**
     * @brief Trigger excitation with current parameters
     *
     * Applies the configured excitation to the modal node.
     */
    void trigger();

    /**
     * @brief Process one control tick (called at 500 Hz)
     *
     * Updates modal dynamics and internal state.
     */
    void processControlTick();

    /**
     * @brief Render audio samples
     * @param outL Left channel output buffer
     * @param outR Right channel output buffer
     * @param frames Number of frames to render
     */
    void renderAudio(float* outL, float* outR, uint32_t frames);

    /**
     * @brief Get underlying modal node (read-only access)
     * @return Const reference to modal node
     */
    const modal_node_t& getNode() const { return node_; }

    /**
     * @brief Get voice ID
     * @return Voice identifier
     */
    uint8_t getVoiceId() const { return voice_id_; }

private:
    /**
     * @brief Update mode frequencies based on pitch and bend
     */
    void updateFrequencies();

    // Voice configuration
    uint8_t voice_id_ = 0;
    float sample_rate_ = 48000.0f;
    float pitch_hz_ = 440.0f;
    float pitch_bend_semis_ = 0.0f;
    float global_damping_ = 0.0f;

    // Excitation state
    ExcitationParams excitation_{};

    // Core DSP structures
    modal_node_t node_{};
    audio_synth_t synth_{};

    // Initialization flag
    bool initialized_ = false;
};
