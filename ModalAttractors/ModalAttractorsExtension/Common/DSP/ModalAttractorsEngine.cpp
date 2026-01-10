/**
 * @file ModalAttractorsEngine.cpp
 * @brief C API implementation wrapping SynthEngine
 *
 * Provides a C-compatible interface for Swift AU wrapper.
 */

#include "ModalAttractorsAU.h"
#include "../../DSP/SynthEngine.h"
#include <cstring>

// Event queue stored in engine for sample-accurate processing
struct EngineState {
    SynthEngine* synthEngine;
    EventQueue eventQueue;
};

void modal_attractors_engine_init(ModalAttractorsEngine* engine,
                                  double sampleRate,
                                  uint32_t maxFrames,
                                  uint32_t maxPolyphony) {
    if (!engine) return;

    // Create engine state
    EngineState* state = new EngineState();
    state->synthEngine = new SynthEngine(maxPolyphony);
    state->synthEngine->prepare(sampleRate, maxFrames, 2); // stereo

    engine->synthEngine = (SynthEngine*)state; // Store as opaque pointer
}

void modal_attractors_engine_prepare(ModalAttractorsEngine* engine,
                                    double sampleRate,
                                    uint32_t maxFrames,
                                    uint32_t maxPolyphony) {
    if (!engine || !engine->synthEngine) return;

    EngineState* state = (EngineState*)engine->synthEngine;
    state->synthEngine->prepare(sampleRate, maxFrames, 2);
}

void modal_attractors_engine_cleanup(ModalAttractorsEngine* engine) {
    if (!engine || !engine->synthEngine) return;

    EngineState* state = (EngineState*)engine->synthEngine;
    delete state->synthEngine;
    delete state;
    engine->synthEngine = nullptr;
}

void modal_attractors_engine_reset(ModalAttractorsEngine* engine) {
    if (!engine || !engine->synthEngine) return;

    EngineState* state = (EngineState*)engine->synthEngine;
    state->synthEngine->reset();
}

void modal_attractors_engine_begin_events(ModalAttractorsEngine* engine) {
    if (!engine || !engine->synthEngine) return;

    EngineState* state = (EngineState*)engine->synthEngine;
    state->eventQueue.clear();
}

void modal_attractors_engine_push_note_on(ModalAttractorsEngine* engine,
                                         int32_t sampleOffset,
                                         uint8_t note,
                                         float velocity) {
    if (!engine || !engine->synthEngine) return;

    EngineState* state = (EngineState*)engine->synthEngine;
    SynthEvent event;
    event.type = EventType::NoteOn;
    event.sampleOffset = sampleOffset;
    event.noteOn.note = note;
    event.noteOn.velocity = velocity;
    event.noteOn.channel = 0;
    state->eventQueue.push(event);
}

void modal_attractors_engine_push_note_off(ModalAttractorsEngine* engine,
                                          int32_t sampleOffset,
                                          uint8_t note) {
    if (!engine || !engine->synthEngine) return;

    EngineState* state = (EngineState*)engine->synthEngine;
    SynthEvent event;
    event.type = EventType::NoteOff;
    event.sampleOffset = sampleOffset;
    event.noteOff.note = note;
    state->eventQueue.push(event);
}

void modal_attractors_engine_push_pitch_bend(ModalAttractorsEngine* engine,
                                            int32_t sampleOffset,
                                            float value) {
    if (!engine || !engine->synthEngine) return;

    EngineState* state = (EngineState*)engine->synthEngine;
    SynthEvent event;
    event.type = EventType::PitchBend;
    event.sampleOffset = sampleOffset;
    event.pitchBend.value = value;
    state->eventQueue.push(event);
}

void modal_attractors_engine_push_parameter(ModalAttractorsEngine* engine,
                                           int32_t sampleOffset,
                                           uint32_t paramId,
                                           float value) {
    if (!engine || !engine->synthEngine) return;

    EngineState* state = (EngineState*)engine->synthEngine;
    SynthEvent event;
    event.type = EventType::Parameter;
    event.sampleOffset = sampleOffset;
    event.parameter.paramId = paramId;
    event.parameter.value = value;
    state->eventQueue.push(event);
}

void modal_attractors_engine_render(ModalAttractorsEngine* engine,
                                   float* outL,
                                   float* outR,
                                   uint32_t numFrames) {
    if (!engine || !engine->synthEngine) {
        // Return silence if not initialized
        memset(outL, 0, numFrames * sizeof(float));
        memset(outR, 0, numFrames * sizeof(float));
        return;
    }

    EngineState* state = (EngineState*)engine->synthEngine;
    state->synthEngine->render(state->eventQueue, outL, outR, numFrames);
}

void modal_attractors_engine_set_parameter(ModalAttractorsEngine* engine,
                                          uint32_t paramId,
                                          float value) {
    if (!engine || !engine->synthEngine) return;

    EngineState* state = (EngineState*)engine->synthEngine;
    state->synthEngine->setParameter(paramId, value);
}

float modal_attractors_engine_get_parameter(ModalAttractorsEngine* engine,
                                            uint32_t paramId) {
    if (!engine || !engine->synthEngine) return 0.0f;

    EngineState* state = (EngineState*)engine->synthEngine;
    return state->synthEngine->getParameter(paramId);
}
