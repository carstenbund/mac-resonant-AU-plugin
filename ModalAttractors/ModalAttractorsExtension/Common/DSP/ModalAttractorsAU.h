/**
 * @file ModalAttractorsAU.h
 * @brief C API bridge between Swift AU wrapper and C++ SynthEngine
 *
 * This provides an Apple-type-free interface that Swift can call.
 * All functions are real-time safe and wrap the SynthEngine class.
 */

#ifndef MODAL_ATTRACTORS_AU_H
#define MODAL_ATTRACTORS_AU_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declare C++ class (opaque pointer for C)
class SynthEngine;

/**
 * @brief DSP engine wrapper (opaque handle for Swift)
 */
struct ModalAttractorsEngine {
    SynthEngine* synthEngine;
};

/**
 * @brief Initialize the DSP engine
 * @param engine Engine handle
 * @param sampleRate Sample rate in Hz
 * @param maxFrames Maximum frames per render call
 * @param maxPolyphony Maximum polyphony (ignored, always 5 nodes)
 */
void modal_attractors_engine_init(ModalAttractorsEngine* engine,
                                  double sampleRate,
                                  uint32_t maxFrames,
                                  uint32_t maxPolyphony);

/**
 * @brief Prepare engine for processing (same as init for now)
 * @param engine Engine handle
 * @param sampleRate Sample rate in Hz
 * @param maxFrames Maximum frames per render call
 * @param maxPolyphony Maximum polyphony
 */
void modal_attractors_engine_prepare(ModalAttractorsEngine* engine,
                                    double sampleRate,
                                    uint32_t maxFrames,
                                    uint32_t maxPolyphony);

/**
 * @brief Clean up engine resources
 * @param engine Engine handle
 */
void modal_attractors_engine_cleanup(ModalAttractorsEngine* engine);

/**
 * @brief Reset engine state
 * @param engine Engine handle
 */
void modal_attractors_engine_reset(ModalAttractorsEngine* engine);

/**
 * @brief Begin event queue for current buffer (clears queue)
 * @param engine Engine handle
 */
void modal_attractors_engine_begin_events(ModalAttractorsEngine* engine);

/**
 * @brief Push note on event to queue
 * @param engine Engine handle
 * @param sampleOffset Sample offset (0 to bufferSize-1)
 * @param note MIDI note number (0-127)
 * @param velocity Velocity (0.0-1.0 normalized)
 */
void modal_attractors_engine_push_note_on(ModalAttractorsEngine* engine,
                                         int32_t sampleOffset,
                                         uint8_t note,
                                         float velocity);

/**
 * @brief Push note off event to queue
 * @param engine Engine handle
 * @param sampleOffset Sample offset (0 to bufferSize-1)
 * @param note MIDI note number (0-127)
 */
void modal_attractors_engine_push_note_off(ModalAttractorsEngine* engine,
                                          int32_t sampleOffset,
                                          uint8_t note);

/**
 * @brief Push pitch bend event to queue
 * @param engine Engine handle
 * @param sampleOffset Sample offset (0 to bufferSize-1)
 * @param value Pitch bend value (-1.0 to +1.0)
 */
void modal_attractors_engine_push_pitch_bend(ModalAttractorsEngine* engine,
                                            int32_t sampleOffset,
                                            float value);

/**
 * @brief Push parameter change event to queue
 * @param engine Engine handle
 * @param sampleOffset Sample offset (0 to bufferSize-1)
 * @param paramId Parameter ID
 * @param value Parameter value
 */
void modal_attractors_engine_push_parameter(ModalAttractorsEngine* engine,
                                           int32_t sampleOffset,
                                           uint32_t paramId,
                                           float value);

/**
 * @brief Render audio with queued events
 * @param engine Engine handle
 * @param outL Left channel output buffer
 * @param outR Right channel output buffer
 * @param numFrames Number of frames to render
 */
void modal_attractors_engine_render(ModalAttractorsEngine* engine,
                                   float* outL,
                                   float* outR,
                                   uint32_t numFrames);

/**
 * @brief Set parameter immediately (non-queued)
 * @param engine Engine handle
 * @param paramId Parameter ID
 * @param value Parameter value
 */
void modal_attractors_engine_set_parameter(ModalAttractorsEngine* engine,
                                          uint32_t paramId,
                                          float value);

/**
 * @brief Get current parameter value
 * @param engine Engine handle
 * @param paramId Parameter ID
 * @return Current parameter value
 */
float modal_attractors_engine_get_parameter(ModalAttractorsEngine* engine,
                                            uint32_t paramId);

#ifdef __cplusplus
}
#endif

#endif // MODAL_ATTRACTORS_AU_H
