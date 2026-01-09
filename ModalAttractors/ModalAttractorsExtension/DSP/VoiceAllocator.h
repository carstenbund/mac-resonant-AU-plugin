/**
 * @file VoiceAllocator.h
 * @brief Polyphonic voice allocation and management
 *
 * Manages a pool of ModalVoice instances for polyphonic synthesis.
 * Handles:
 * - Note on/off events
 * - Voice stealing (when all voices are in use)
 * - MIDI note → voice mapping
 */

#ifndef VOICE_ALLOCATOR_H
#define VOICE_ALLOCATOR_H

#include "ModalVoice.h"
#include <cstdint>

/**
 * @brief Default maximum polyphony
 */
#define DEFAULT_MAX_POLYPHONY 16

class VoiceAllocator {
public:
    /**
     * @brief Constructor
     * @param max_polyphony Maximum number of simultaneous voices
     */
    VoiceAllocator(uint32_t max_polyphony = DEFAULT_MAX_POLYPHONY);

    /**
     * @brief Destructor
     */
    ~VoiceAllocator();

    /**
     * @brief Initialize allocator
     * @param sample_rate Sample rate in Hz
     */
    void initialize(float sample_rate);

    /**
     * @brief Handle MIDI note on
     * @param midi_note MIDI note number (0-127)
     * @param velocity Velocity (0.0-1.0 normalized)
     * @return Pointer to allocated voice, or nullptr if allocation failed
     */
    ModalVoice* noteOn(uint8_t midi_note, float velocity);

    /**
     * @brief Handle MIDI note off
     * @param midi_note MIDI note number (0-127)
     */
    void noteOff(uint8_t midi_note);

    /**
     * @brief Release all voices
     */
    void allNotesOff();

    /**
     * @brief Apply pitch bend to all active voices
     * @param bend_amount Pitch bend amount (-1.0 to +1.0)
     */
    void setPitchBend(float bend_amount);

    /**
     * @brief Set personality for all voices
     * @param personality Personality mode (0 = Resonator, 1 = Self-oscillator)
     */
    void setPersonality(node_personality_t personality);

    /**
     * @brief Set mode parameters for all voices
     * @param mode_idx Mode index (0-3)
     * @param freq_multiplier Frequency multiplier relative to base note
     * @param damping Damping coefficient
     * @param weight Audio weight (0.0-1.0)
     */
    void setMode(uint8_t mode_idx, float freq_multiplier, float damping, float weight);

    /**
     * @brief Set poke excitation strength
     * @param strength Poke strength (0.0-1.0)
     */
    void setPokeStrength(float strength);

    /**
     * @brief Set poke excitation duration
     * @param duration_ms Duration in milliseconds
     */
    void setPokeDuration(float duration_ms);

    /**
     * @brief Set maximum number of active nodes/voices
     * @param node_count Number of active nodes (1-16)
     */
    void setNodeCount(uint32_t node_count);

    /**
     * @brief Update all active voices (control rate)
     *
     * Should be called at control rate (500 Hz typically)
     */
    void updateVoices();

    /**
     * @brief Render audio from all active voices
     * @param outL Left channel output buffer
     * @param outR Right channel output buffer
     * @param num_frames Number of frames to render
     */
    void renderAudio(float* outL, float* outR, uint32_t num_frames);

    /**
     * @brief Get voice by index
     * @param voice_idx Voice index [0..max_polyphony-1]
     * @return Pointer to voice, or nullptr if invalid index
     */
    ModalVoice* getVoice(uint32_t voice_idx);

    /**
     * @brief Get maximum polyphony
     * @return Maximum number of voices
     */
    uint32_t getMaxPolyphony() const { return max_polyphony_; }

    /**
     * @brief Get number of active voices
     * @return Number of currently active voices
     */
    uint32_t getActiveVoiceCount() const;

private:
    ModalVoice** voices_;              ///< Voice pool
    uint32_t max_polyphony_;           ///< Maximum polyphony
    uint32_t active_node_count_;       ///< Current active node count (1-max_polyphony_)

    int8_t note_to_voice_[128];        ///< MIDI note → voice mapping (-1 = none)
    float pitch_bend_;                 ///< Current pitch bend amount
    node_personality_t personality_;   ///< Current personality mode

    // Mode parameters (for each of 4 modes)
    struct ModeParams {
        float freq_multiplier;
        float damping;
        float weight;
    };
    ModeParams mode_params_[4];        ///< Current mode parameters

    // Excitation parameters
    float poke_strength_;              ///< Poke strength (0.0-1.0)
    float poke_duration_ms_;           ///< Poke duration in milliseconds

    // Pre-allocated temp buffers for render (real-time safe)
    float* temp_buffer_L_;             ///< Temp buffer for voice rendering (L channel)
    float* temp_buffer_R_;             ///< Temp buffer for voice rendering (R channel)
    uint32_t max_buffer_size_;         ///< Maximum buffer size allocated

    float sample_rate_;                ///< Current sample rate
    bool initialized_;                 ///< Initialization flag

    /**
     * @brief Find free voice for allocation
     * @return Pointer to free voice, or nullptr if none available
     */
    ModalVoice* findFreeVoice();

    /**
     * @brief Steal oldest voice
     * @return Pointer to stolen voice
     */
    ModalVoice* stealOldestVoice();
};

#endif // VOICE_ALLOCATOR_H
