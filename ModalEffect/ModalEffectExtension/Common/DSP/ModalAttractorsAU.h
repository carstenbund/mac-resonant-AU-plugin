/**
 * @file ModalAttractorsAU.h
 * @brief Bridge between AU wrapper and C++ DSP engine
 *
 * Provides a C-compatible interface for the AU wrapper to call the C++ DSP engine.
 * This file is Apple-type-free and can be included from both C++ and Objective-C++.
 *
 * Architecture:
 * - AU wrapper handles AURenderEvent parsing
 * - Converts events to SynthEvent and pushes to EventQueue
 * - Calls render with event queue for sample-accurate processing
 */

#ifndef MODAL_ATTRACTORS_AU_H
#define MODAL_ATTRACTORS_AU_H

#include <cstdint>

// Forward declaration - actual definition in SynthEngine.h
class SynthEngine;
class EventQueue;

/**
 * @brief Opaque engine handle
 *
 * The AU wrapper holds a pointer to this struct.
 * Internally it wraps a C++ SynthEngine instance.
 */
struct ModalAttractorsEngine {
    SynthEngine* synth_engine;  // C++ engine (opaque to C callers)
    EventQueue* event_queue;    // Render-time event queue
    bool initialized;
};

// ============================================================================
// C-compatible API for AU wrapper
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the DSP engine
 * @param engine Engine handle
 * @param sample_rate Sample rate in Hz
 * @param max_frames Maximum frames per render call
 * @param max_polyphony Maximum number of voices
 */
void modal_attractors_engine_init(ModalAttractorsEngine* engine,
                                  double sample_rate,
                                  uint32_t max_frames,
                                  uint32_t max_polyphony);

/**
 * @brief Prepare engine (call when sample rate changes)
 * @param engine Engine handle
 * @param sample_rate Sample rate in Hz
 * @param max_frames Maximum frames per render call
 */
void modal_attractors_engine_prepare(ModalAttractorsEngine* engine,
                                     double sample_rate,
                                     uint32_t max_frames);

/**
 * @brief Reset engine state
 * @param engine Engine handle
 */
void modal_attractors_engine_reset(ModalAttractorsEngine* engine);

/**
 * @brief Clean up engine resources
 * @param engine Engine handle
 */
void modal_attractors_engine_cleanup(ModalAttractorsEngine* engine);

// ============================================================================
// Event handling (for sample-accurate MIDI/automation)
// ============================================================================

/**
 * @brief Begin event frame (clear event queue)
 * Call this at the start of each render call.
 */
void modal_attractors_engine_begin_events(ModalAttractorsEngine* engine);

/**
 * @brief Push note on event
 * @param sample_offset Sample offset in current buffer (0 to num_frames-1)
 * @param note MIDI note number (0-127)
 * @param velocity Normalized velocity (0.0-1.0)
 * @param channel MIDI channel (0-15, where 0 = channel 1)
 */
void modal_attractors_engine_push_note_on(ModalAttractorsEngine* engine,
                                          int32_t sample_offset,
                                          uint8_t note,
                                          float velocity,
                                          uint8_t channel);

/**
 * @brief Push note off event
 * @param sample_offset Sample offset in current buffer
 * @param note MIDI note number (0-127)
 */
void modal_attractors_engine_push_note_off(ModalAttractorsEngine* engine,
                                           int32_t sample_offset,
                                           uint8_t note);

/**
 * @brief Push pitch bend event
 * @param sample_offset Sample offset in current buffer
 * @param value Pitch bend value (-1.0 to +1.0)
 */
void modal_attractors_engine_push_pitch_bend(ModalAttractorsEngine* engine,
                                             int32_t sample_offset,
                                             float value);

/**
 * @brief Push parameter change event
 * @param sample_offset Sample offset in current buffer
 * @param param_id Parameter ID
 * @param value Parameter value
 */
void modal_attractors_engine_push_parameter(ModalAttractorsEngine* engine,
                                            int32_t sample_offset,
                                            uint32_t param_id,
                                            float value);

// ============================================================================
// Rendering
// ============================================================================

/**
 * @brief Render audio with queued events
 *
 * Process all events pushed since begin_events() call, applying them
 * at their sample offsets for sample-accurate timing.
 *
 * @param engine Engine handle
 * @param outL Left channel output buffer
 * @param outR Right channel output buffer
 * @param num_frames Number of frames to render
 */
void modal_attractors_engine_render(ModalAttractorsEngine* engine,
                                    float* outL,
                                    float* outR,
                                    uint32_t num_frames);

// ============================================================================
// Parameter access (for host automation)
// ============================================================================

/**
 * @brief Set parameter immediately (not sample-accurate)
 * Use push_parameter for sample-accurate automation
 */
void modal_attractors_engine_set_parameter(ModalAttractorsEngine* engine,
                                           uint32_t param_id,
                                           float value);

/**
 * @brief Get parameter value
 */
float modal_attractors_engine_get_parameter(ModalAttractorsEngine* engine,
                                            uint32_t param_id);

#ifdef __cplusplus
}
#endif

#endif // MODAL_ATTRACTORS_AU_H
