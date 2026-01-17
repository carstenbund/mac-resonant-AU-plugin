/**
 * @file usb_midi_handler.h
 * @brief USB MIDI input handler for Daisy Seed
 *
 * Maps MIDI messages to modal node control:
 * - Note On → Trigger poke event
 * - Note Off → Optional decay modulation
 * - CC → Mode parameter control (damping, coupling, etc.)
 * - Pitch Bend → Global frequency modulation
 */

#ifndef USB_MIDI_HANDLER_H
#define USB_MIDI_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "../core/modal_node.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// MIDI CC Mapping
// ============================================================================

// Standard MIDI CC assignments
#define MIDI_CC_MOD_WHEEL      1
#define MIDI_CC_BREATH         2
#define MIDI_CC_EXPRESSION     11
#define MIDI_CC_DAMPING        64   // Sustain pedal repurposed
#define MIDI_CC_RESONANCE      71
#define MIDI_CC_BRIGHTNESS     74
#define MIDI_CC_MODE0_GAIN     20
#define MIDI_CC_MODE1_GAIN     21
#define MIDI_CC_MODE2_GAIN     22
#define MIDI_CC_MODE3_GAIN     23

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief MIDI handler state
 */
typedef struct {
    modal_node_t* node;              ///< Target modal node
    float velocity_scale;            ///< Velocity to poke strength scaling
    bool note_on_triggers_poke;      ///< Note-on triggers poke (vs continuous)
    uint8_t last_note;               ///< Last played note
    float last_velocity;             ///< Last note velocity (normalized)
} midi_handler_t;

// ============================================================================
// Core API
// ============================================================================

/**
 * @brief Initialize MIDI handler
 *
 * @param handler Pointer to MIDI handler state
 * @param node Pointer to modal node to control
 */
void midi_handler_init(midi_handler_t* handler, modal_node_t* node);

/**
 * @brief Process MIDI Note On message
 *
 * Triggers poke event with velocity-scaled strength.
 * Optionally updates modal frequencies based on note.
 *
 * @param handler Pointer to MIDI handler
 * @param note MIDI note number [0-127]
 * @param velocity MIDI velocity [0-127]
 */
void midi_handler_note_on(midi_handler_t* handler, uint8_t note, uint8_t velocity);

/**
 * @brief Process MIDI Note Off message
 *
 * @param handler Pointer to MIDI handler
 * @param note MIDI note number [0-127]
 */
void midi_handler_note_off(midi_handler_t* handler, uint8_t note);

/**
 * @brief Process MIDI Control Change message
 *
 * Maps CC to modal parameters:
 * - CC 1 (Mod Wheel) → Mode 0 damping
 * - CC 71 (Resonance) → Global coupling strength
 * - CC 74 (Brightness) → Mode 2 gain
 * - CC 20-23 → Individual mode gains
 *
 * @param handler Pointer to MIDI handler
 * @param cc_number CC number [0-127]
 * @param cc_value CC value [0-127]
 */
void midi_handler_cc(midi_handler_t* handler, uint8_t cc_number, uint8_t cc_value);

/**
 * @brief Process MIDI Pitch Bend message
 *
 * Modulates all mode frequencies proportionally.
 *
 * @param handler Pointer to MIDI handler
 * @param bend Pitch bend value [-8192 to 8191]
 */
void midi_handler_pitch_bend(midi_handler_t* handler, int16_t bend);

/**
 * @brief Set velocity scaling factor
 *
 * @param handler Pointer to MIDI handler
 * @param scale Velocity scale [0.0-2.0] (1.0 = linear)
 */
void midi_handler_set_velocity_scale(midi_handler_t* handler, float scale);

#ifdef __cplusplus
}
#endif

#endif // USB_MIDI_HANDLER_H
