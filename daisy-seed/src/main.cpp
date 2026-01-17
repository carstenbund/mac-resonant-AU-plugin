/**
 * @file main.cpp
 * @brief Daisy Seed Modal Resonator - Main Entry Point
 *
 * Single-node modal resonator with USB MIDI control.
 * Port of ESP32 distributed resonator network to Daisy Seed platform.
 *
 * Hardware: Electrosmith Daisy Seed
 * Features:
 * - 4-mode modal resonator
 * - USB MIDI input (note-on triggers pokes)
 * - Stereo audio output via WM8731 codec
 * - 48kHz sample rate
 * - 500Hz control rate (modal integration)
 *
 * Author: Adapted from ESP32 implementation
 * Date: 2026-01-17
 */

#include "daisy_seed.h"

extern "C" {
#include "core/modal_node.h"
#include "audio/audio_synth.h"
#include "midi/usb_midi_handler.h"
}

using namespace daisy;

// ============================================================================
// Global State
// ============================================================================

DaisySeed hw;
modal_node_t node;
audio_synth_t synth;
midi_handler_t midi_handler;

// Control rate timing
uint32_t last_control_ms = 0;
const uint32_t CONTROL_PERIOD_MS = 2;  // 500Hz control rate

// LED blink for poke events
bool led_state = false;
uint32_t last_poke_ms = 0;

// ============================================================================
// Audio Callback
// ============================================================================

/**
 * @brief Daisy audio callback (called at 48kHz, DMA interrupt context)
 *
 * Generates stereo audio from modal state.
 * Must be fast and lock-free!
 */
void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {
    // Generate stereo audio from modal synthesis
    audio_synth_process(&synth, out[0], out[1], size);
}

// ============================================================================
// MIDI Processing
// ============================================================================

/**
 * @brief Process incoming USB MIDI messages
 */
void ProcessMIDI() {
    hw.midi.Listen();

    while (hw.midi.HasEvents()) {
        auto midi_event = hw.midi.PopEvent();

        switch (midi_event.type) {
            case NoteOn: {
                uint8_t note = midi_event.AsNoteOn().note;
                uint8_t velocity = midi_event.AsNoteOn().velocity;

                if (velocity > 0) {
                    midi_handler_note_on(&midi_handler, note, velocity);

                    // Blink LED on poke
                    led_state = true;
                    last_poke_ms = System::GetNow();
                } else {
                    // Velocity 0 = note off
                    midi_handler_note_off(&midi_handler, note);
                }
                break;
            }

            case NoteOff: {
                uint8_t note = midi_event.AsNoteOff().note;
                midi_handler_note_off(&midi_handler, note);
                break;
            }

            case ControlChange: {
                uint8_t cc_num = midi_event.AsControlChange().control_number;
                uint8_t cc_val = midi_event.AsControlChange().value;
                midi_handler_cc(&midi_handler, cc_num, cc_val);
                break;
            }

            case PitchBend: {
                int16_t bend = midi_event.AsPitchBend().value;
                midi_handler_pitch_bend(&midi_handler, bend);
                break;
            }

            default:
                break;
        }
    }
}

// ============================================================================
// Control Rate Processing
// ============================================================================

/**
 * @brief Run modal integration at control rate (500Hz)
 */
void ProcessControl() {
    uint32_t now = System::GetNow();

    if (now - last_control_ms >= CONTROL_PERIOD_MS) {
        // Integrate modal dynamics
        modal_node_step(&node);

        last_control_ms = now;
    }

    // LED blink on poke (100ms)
    if (led_state && (now - last_poke_ms > 100)) {
        led_state = false;
    }

    hw.SetLed(led_state);
}

// ============================================================================
// Initialization
// ============================================================================

/**
 * @brief Initialize hardware and DSP
 */
void setup() {
    // Initialize Daisy Seed hardware
    hw.Init();
    hw.SetAudioBlockSize(48);  // 48 samples = 1ms @ 48kHz
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    // Initialize modal node with 4 modes
    modal_node_init(&node, 0, PERSONALITY_RESONATOR);

    // Configure modal frequencies (example: harmonic series from A2)
    // Mode 0: Fundamental (110 Hz)
    modal_node_set_mode(&node, 0,
                       freq_to_omega(110.0f),
                       0.5f,   // Damping (1/sec)
                       1.0f);  // Weight

    // Mode 1: Octave + slight detune (220.5 Hz)
    modal_node_set_mode(&node, 1,
                       freq_to_omega(220.5f),
                       0.6f,
                       0.8f);

    // Mode 2: 3rd harmonic (330 Hz) - brightness
    modal_node_set_mode(&node, 2,
                       freq_to_omega(330.0f),
                       1.0f,   // Faster decay
                       0.5f);

    // Mode 3: Sub-bass (55 Hz)
    modal_node_set_mode(&node, 3,
                       freq_to_omega(55.0f),
                       0.3f,   // Slow decay
                       0.6f);

    // Start node
    modal_node_start(&node);

    // Initialize audio synthesis
    audio_synth_init(&synth, &node);

    // Initialize MIDI handler
    midi_handler_init(&midi_handler, &node);

    // Start audio
    hw.StartAudio(AudioCallback);

    // Start MIDI
    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
    hw.midi.Init(midi_cfg);

    // Seed random for phase randomization
    srand(System::GetNow());

    // LED on to indicate ready
    hw.SetLed(true);
    System::Delay(500);
    hw.SetLed(false);
}

// ============================================================================
// Main Loop
// ============================================================================

/**
 * @brief Main loop (runs continuously)
 *
 * Handles:
 * - Modal integration at 500Hz
 * - MIDI input processing
 * - LED status updates
 */
void loop() {
    // Process MIDI events
    ProcessMIDI();

    // Run control rate processing
    ProcessControl();

    // Small delay to prevent busy-wait
    System::Delay(1);
}

// ============================================================================
// Arduino Entry Points
// ============================================================================

int main(void) {
    setup();

    while (1) {
        loop();
    }
}
