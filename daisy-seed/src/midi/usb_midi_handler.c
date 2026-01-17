/**
 * @file usb_midi_handler.c
 * @brief USB MIDI input handler implementation
 */

#include "usb_midi_handler.h"
#include <math.h>
#include <string.h>

// ============================================================================
// Initialization
// ============================================================================

void midi_handler_init(midi_handler_t* handler, modal_node_t* node) {
    memset(handler, 0, sizeof(midi_handler_t));

    handler->node = node;
    handler->velocity_scale = 1.0f;
    handler->note_on_triggers_poke = true;
    handler->last_note = 60;  // Middle C
    handler->last_velocity = 0.7f;
}

// ============================================================================
// MIDI Message Handlers
// ============================================================================

void midi_handler_note_on(midi_handler_t* handler, uint8_t note, uint8_t velocity) {
    if (!handler->node || velocity == 0) return;

    handler->last_note = note;
    handler->last_velocity = velocity / 127.0f;

    // Trigger poke event
    if (handler->note_on_triggers_poke) {
        poke_event_t poke;
        poke.source_node_id = 0;  // Self (single node mode)

        // Velocity to strength (with scaling)
        poke.strength = (velocity / 127.0f) * handler->velocity_scale;

        // Clamp strength
        if (poke.strength > 1.0f) poke.strength = 1.0f;

        // Random phase for organic sound
        poke.phase_hint = -1.0f;

        // Mode weights: emphasize lower modes for strong hits
        // Higher notes → emphasize higher modes
        float note_norm = (note - 36.0f) / 48.0f;  // 0 at C1, 1 at C5
        if (note_norm < 0.0f) note_norm = 0.0f;
        if (note_norm > 1.0f) note_norm = 1.0f;

        poke.mode_weights[0] = 1.0f;
        poke.mode_weights[1] = 0.8f + 0.2f * note_norm;
        poke.mode_weights[2] = 0.5f + 0.5f * note_norm;
        poke.mode_weights[3] = 0.3f + 0.7f * note_norm;

        modal_node_apply_poke(handler->node, &poke);
    }

    // Optional: Update mode frequencies based on note
    // This creates pitched behavior (vs fixed resonances)
    #ifdef MIDI_NOTE_PITCH_TRACKING
    float base_freq = midi_to_freq(note);

    // Harmonic series: f0, f0*2, f0*3, f0*4
    modal_node_set_mode(handler->node, 0,
                       freq_to_omega(base_freq),
                       handler->node->modes[0].params.gamma,
                       handler->node->modes[0].params.weight);

    modal_node_set_mode(handler->node, 1,
                       freq_to_omega(base_freq * 2.01f),  // Slight detune
                       handler->node->modes[1].params.gamma,
                       handler->node->modes[1].params.weight);

    modal_node_set_mode(handler->node, 2,
                       freq_to_omega(base_freq * 3.02f),
                       handler->node->modes[2].params.gamma,
                       handler->node->modes[2].params.weight);

    modal_node_set_mode(handler->node, 3,
                       freq_to_omega(base_freq * 0.5f),  // Sub-bass
                       handler->node->modes[3].params.gamma,
                       handler->node->modes[3].params.weight);
    #endif
}

void midi_handler_note_off(midi_handler_t* handler, uint8_t note) {
    // For resonator personality, note-off doesn't do much
    // (modes decay naturally)

    // Could optionally increase damping for faster decay:
    #ifdef MIDI_NOTE_OFF_DECAY
    if (handler->node && note == handler->last_note) {
        for (int k = 0; k < MAX_MODES; k++) {
            if (handler->node->modes[k].params.active) {
                handler->node->modes[k].params.gamma *= 1.5f;  // Faster decay
            }
        }
    }
    #endif
}

void midi_handler_cc(midi_handler_t* handler, uint8_t cc_number, uint8_t cc_value) {
    if (!handler->node) return;

    float value_norm = cc_value / 127.0f;

    switch (cc_number) {
        case MIDI_CC_MOD_WHEEL:
            // Modulation wheel → Mode 0 damping
            // Lower mod = less damping (longer decay)
            if (handler->node->modes[0].params.active) {
                float gamma = 0.2f + value_norm * 2.0f;  // 0.2 to 2.2
                handler->node->modes[0].params.gamma = gamma;
            }
            break;

        case MIDI_CC_BREATH:
            // Breath controller → Global coupling strength
            handler->node->coupling_strength = value_norm;
            break;

        case MIDI_CC_RESONANCE:
            // Resonance → Reduce all damping (increase resonance)
            for (int k = 0; k < MAX_MODES; k++) {
                if (handler->node->modes[k].params.active) {
                    // More resonance = less damping
                    float base_gamma = 1.0f;
                    handler->node->modes[k].params.gamma =
                        base_gamma * (1.0f - 0.7f * value_norm);
                }
            }
            break;

        case MIDI_CC_BRIGHTNESS:
            // Brightness → Mode 2 gain (high frequency mode)
            if (handler->node->modes[2].params.active) {
                handler->node->modes[2].params.weight = value_norm;
            }
            break;

        case MIDI_CC_MODE0_GAIN:
            if (handler->node->modes[0].params.active) {
                handler->node->modes[0].params.weight = value_norm;
            }
            break;

        case MIDI_CC_MODE1_GAIN:
            if (handler->node->modes[1].params.active) {
                handler->node->modes[1].params.weight = value_norm;
            }
            break;

        case MIDI_CC_MODE2_GAIN:
            if (handler->node->modes[2].params.active) {
                handler->node->modes[2].params.weight = value_norm;
            }
            break;

        case MIDI_CC_MODE3_GAIN:
            if (handler->node->modes[3].params.active) {
                handler->node->modes[3].params.weight = value_norm;
            }
            break;

        default:
            // Unhandled CC
            break;
    }
}

void midi_handler_pitch_bend(midi_handler_t* handler, int16_t bend) {
    if (!handler->node) return;

    // Pitch bend range: ±2 semitones
    float semitones = (bend / 8192.0f) * 2.0f;
    float freq_ratio = powf(2.0f, semitones / 12.0f);

    // Apply to all active modes
    for (int k = 0; k < MAX_MODES; k++) {
        if (handler->node->modes[k].params.active) {
            float base_omega = handler->node->modes[k].params.omega / freq_ratio;
            handler->node->modes[k].params.omega = base_omega * freq_ratio;
        }
    }
}

void midi_handler_set_velocity_scale(midi_handler_t* handler, float scale) {
    if (scale < 0.0f) scale = 0.0f;
    if (scale > 2.0f) scale = 2.0f;
    handler->velocity_scale = scale;
}
