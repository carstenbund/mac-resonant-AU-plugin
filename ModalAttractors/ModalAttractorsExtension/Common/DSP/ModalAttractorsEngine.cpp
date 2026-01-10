/**
 * @file ModalAttractorsEngine.cpp
 * @brief C API implementation for Modal Attractors DSP engine
 *
 * Provides a C-compatible interface between the AU wrapper and C++ SynthEngine.
 * All Apple types stay in the AU wrapper; this file is Apple-type-free.
 */

#include "ModalAttractorsAU.h"
#include "../../DSP/SynthEngine.h"
#include <cstring>

// ============================================================================
// Initialization and cleanup
// ============================================================================

void modal_attractors_engine_init(ModalAttractorsEngine* engine,
                                  double sample_rate,
                                  uint32_t max_frames,
                                  uint32_t max_polyphony) {
    if (!engine) return;

    memset(engine, 0, sizeof(ModalAttractorsEngine));

    // Create C++ engine and event queue (only allocation happens here, not in render!)
    engine->synth_engine = new SynthEngine(max_polyphony);
    engine->event_queue = new EventQueue();

    // Prepare engine for processing
    engine->synth_engine->prepare(sample_rate, max_frames, 2);

    engine->initialized = true;
}

void modal_attractors_engine_prepare(ModalAttractorsEngine* engine,
                                     double sample_rate,
                                     uint32_t max_frames) {
    if (!engine || !engine->initialized) return;

    engine->synth_engine->prepare(sample_rate, max_frames, 2);
}

void modal_attractors_engine_reset(ModalAttractorsEngine* engine) {
    if (!engine || !engine->initialized) return;

    engine->synth_engine->reset();
}

void modal_attractors_engine_cleanup(ModalAttractorsEngine* engine) {
    if (!engine) return;

    if (engine->synth_engine) {
        delete engine->synth_engine;
        engine->synth_engine = nullptr;
    }

    if (engine->event_queue) {
        delete engine->event_queue;
        engine->event_queue = nullptr;
    }

    engine->initialized = false;
}

// ============================================================================
// Event handling (real-time safe)
// ============================================================================

void modal_attractors_engine_begin_events(ModalAttractorsEngine* engine) {
    if (!engine || !engine->initialized) return;

    // Clear event queue for this render frame
    engine->event_queue->clear();
}

void modal_attractors_engine_push_note_on(ModalAttractorsEngine* engine,
                                          int32_t sample_offset,
                                          uint8_t note,
                                          float velocity,
                                          uint8_t channel) {
    if (!engine || !engine->initialized) return;

    SynthEvent event;
    event.type = EventType::NoteOn;
    event.sampleOffset = sample_offset;
    event.noteOn.note = note;
    event.noteOn.velocity = velocity;
    event.noteOn.channel = channel;

    engine->event_queue->push(event);
}

void modal_attractors_engine_push_note_off(ModalAttractorsEngine* engine,
                                           int32_t sample_offset,
                                           uint8_t note) {
    if (!engine || !engine->initialized) return;

    SynthEvent event;
    event.type = EventType::NoteOff;
    event.sampleOffset = sample_offset;
    event.noteOff.note = note;

    engine->event_queue->push(event);
}

void modal_attractors_engine_push_pitch_bend(ModalAttractorsEngine* engine,
                                             int32_t sample_offset,
                                             float value) {
    if (!engine || !engine->initialized) return;

    SynthEvent event;
    event.type = EventType::PitchBend;
    event.sampleOffset = sample_offset;
    event.pitchBend.value = value;

    engine->event_queue->push(event);
}

void modal_attractors_engine_push_parameter(ModalAttractorsEngine* engine,
                                            int32_t sample_offset,
                                            uint32_t param_id,
                                            float value) {
    if (!engine || !engine->initialized) return;

    SynthEvent event;
    event.type = EventType::Parameter;
    event.sampleOffset = sample_offset;
    event.parameter.paramId = param_id;
    event.parameter.value = value;

    engine->event_queue->push(event);
}

// ============================================================================
// Rendering (real-time safe)
// ============================================================================

void modal_attractors_engine_render(ModalAttractorsEngine* engine,
                                    float* outL,
                                    float* outR,
                                    uint32_t num_frames) {
    if (!engine || !engine->initialized) {
        // Return silence if not initialized
        memset(outL, 0, num_frames * sizeof(float));
        if (outR != outL) {
            memset(outR, 0, num_frames * sizeof(float));
        }
        return;
    }

    // Render with sample-accurate event processing
    engine->synth_engine->render(*engine->event_queue, outL, outR, num_frames);
}

// ============================================================================
// Parameter access
// ============================================================================

void modal_attractors_engine_set_parameter(ModalAttractorsEngine* engine,
                                           uint32_t param_id,
                                           float value) {
    if (!engine || !engine->initialized) return;

    engine->synth_engine->setParameter(param_id, value);
}

float modal_attractors_engine_get_parameter(ModalAttractorsEngine* engine,
                                            uint32_t param_id) {
    if (!engine || !engine->initialized) return 0.0f;

    return engine->synth_engine->getParameter(param_id);
}
