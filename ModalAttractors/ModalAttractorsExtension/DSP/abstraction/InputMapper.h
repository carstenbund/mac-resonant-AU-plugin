/**
 * @file InputMapper.h
 * @brief Protocol-agnostic input mapper for MIDI and CV/Gate control
 *
 * Translates both MIDI and CV/Gate inputs into a unified IntentState
 * that can be committed to a ModalVoiceCore. This allows the same
 * mapping logic to work across different input sources (AU MIDI events,
 * ESP32 CV inputs, etc.).
 */

#pragma once

#include <cstdint>
#include "ModalVoiceCore.h"

/**
 * @brief Unified control intent state
 *
 * Represents the user's musical intent abstracted from input protocol.
 * This state is accumulated from MIDI or CV inputs, then committed
 * to the voice at control rate (500 Hz).
 */
struct IntentState {
    float pitch_hz = 440.0f;        ///< Fundamental pitch in Hz
    float energy = 0.0f;            ///< Excitation energy [0..1]
    float duration_ms = 8.0f;       ///< Poke duration in milliseconds
    float brightness = 0.5f;        ///< Brightness/timbre control [0..1]
    float global_damping = 0.0f;    ///< Global damping coefficient [0..1]
    float pitch_bend_semis = 0.0f;  ///< Pitch bend in semitones
    bool gate = false;              ///< Gate state (on/off)
    bool trig_edge = false;         ///< Trigger edge detected this tick
    float phase_hint = -1.0f;       ///< Phase hint for excitation (radians, -1 = random)
};

/**
 * @brief Input mapper for protocol-agnostic control
 *
 * Accepts inputs from MIDI or CV sources and maintains a unified
 * IntentState that can be committed to a ModalVoiceCore.
 */
class InputMapper {
public:
    /**
     * @brief Default constructor
     */
    InputMapper() = default;

    // ========================================================================
    // MIDI Input Methods
    // ========================================================================

    /**
     * @brief Handle MIDI Note On event
     * @param note MIDI note number [0..127]
     * @param velocity MIDI velocity [0..127]
     */
    void onMidiNoteOn(uint8_t note, uint8_t velocity);

    /**
     * @brief Handle MIDI Note Off event
     * @param note MIDI note number [0..127]
     */
    void onMidiNoteOff(uint8_t note);

    /**
     * @brief Handle MIDI pitch bend
     * @param semis Pitch bend in semitones (typically +/- 2 or +/- 12)
     */
    void onMidiPitchBend(float semis);

    /**
     * @brief Handle MIDI CC (Control Change)
     * @param cc CC number [0..127]
     * @param value CC value [0..127]
     */
    void onMidiCC(uint8_t cc, uint8_t value);

    // ========================================================================
    // CV Input Methods
    // ========================================================================

    /**
     * @brief Set CV pitch input (1V/octave standard)
     * @param volts CV pitch voltage
     */
    void setCvPitchVolts(float volts);

    /**
     * @brief Set CV energy input
     * @param volts CV energy voltage [0..5V typical]
     */
    void setCvEnergyVolts(float volts);

    /**
     * @brief Set CV gate input
     * @param volts CV gate voltage (>2.5V = high)
     */
    void setCvGateVolts(float volts);

    /**
     * @brief Set CV trigger input
     * @param volts CV trigger voltage (>2.5V = high)
     */
    void setCvTrigVolts(float volts);

    /**
     * @brief Set CV damping control
     * @param volts CV damping voltage [0..5V]
     */
    void setCvDampVolts(float volts);

    /**
     * @brief Set CV brightness control
     * @param volts CV brightness voltage [0..5V]
     */
    void setCvBrightVolts(float volts);

    // ========================================================================
    // Control Output
    // ========================================================================

    /**
     * @brief Commit current intent state to voice (called at 500 Hz)
     *
     * Updates the voice with accumulated control state and triggers
     * excitation if a trigger edge was detected.
     *
     * @param voice Modal voice to control
     */
    void commitToVoice(ModalVoiceCore& voice);

    /**
     * @brief Get current intent state (read-only)
     * @return Current intent state
     */
    const IntentState& getIntent() const { return intent_; }

private:
    /**
     * @brief Update mode weights based on brightness parameter
     *
     * Maps brightness to mode weight distribution (bright = more high modes).
     */
    void updateModeWeights();

    /**
     * @brief Edge detection for trigger signal
     *
     * Detects rising edge on gate or explicit trigger input.
     */
    void edgeDetectTrigger();

    // Intent state
    IntentState intent_{};

    // Edge detection state
    bool last_gate_ = false;
    bool last_trig_ = false;

    // MIDI note tracking
    uint8_t current_note_ = 60; // Middle C
    bool note_active_ = false;

    // Mode weights (cached for efficiency)
    float mode_weights_[MAX_MODES] = {1.0f, 1.0f, 1.0f, 1.0f};
};
