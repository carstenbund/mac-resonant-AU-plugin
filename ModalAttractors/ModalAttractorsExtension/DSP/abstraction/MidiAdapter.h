/**
 * @file MidiAdapter.h
 * @brief Thin adapter for MIDI input to InputMapper
 *
 * Provides a simple pass-through interface for MIDI events to the
 * protocol-agnostic InputMapper. This adapter exists to provide a
 * clean separation between MIDI-specific event handling and the
 * unified control mapping layer.
 */

#pragma once

#include <cstdint>
#include "InputMapper.h"

/**
 * @brief MIDI input adapter
 *
 * Thin wrapper that forwards MIDI events to an InputMapper instance.
 * Can be used by AU, VST, or other MIDI-based host environments.
 */
class MidiAdapter {
public:
    /**
     * @brief Construct adapter with reference to mapper
     * @param mapper Input mapper to forward events to
     */
    explicit MidiAdapter(InputMapper& mapper)
        : mapper_(mapper)
    {}

    /**
     * @brief Handle MIDI Note On event
     * @param note MIDI note number [0..127]
     * @param velocity MIDI velocity [0..127]
     */
    void handleNoteOn(uint8_t note, uint8_t velocity) {
        mapper_.onMidiNoteOn(note, velocity);
    }

    /**
     * @brief Handle MIDI Note Off event
     * @param note MIDI note number [0..127]
     */
    void handleNoteOff(uint8_t note) {
        mapper_.onMidiNoteOff(note);
    }

    /**
     * @brief Handle MIDI pitch bend
     * @param semis Pitch bend in semitones
     */
    void handlePitchBend(float semis) {
        mapper_.onMidiPitchBend(semis);
    }

    /**
     * @brief Handle MIDI Control Change (CC)
     * @param cc CC number [0..127]
     * @param value CC value [0..127]
     */
    void handleCC(uint8_t cc, uint8_t value) {
        mapper_.onMidiCC(cc, value);
    }

private:
    InputMapper& mapper_;
};
