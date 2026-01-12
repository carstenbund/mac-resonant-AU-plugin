/**
 * @file ModalEffectEngine.cpp
 * @brief C API implementation for Modal Attractors DSP engine
 *
 * Provides a C-compatible interface between the AU wrapper and C++ SynthEngine.
 * All Apple types stay in the AU wrapper; this file is Apple-type-free.
 */

#include "ModalEffectAU.h"
#include "../../DSP/SynthEngine.h"
#include <cstring>

// ============================================================================
// Initialization and cleanup
// ============================================================================

void modal_attractors_engine_init(ModalEffectEngine* engine,
                                  double sample_rate,
                                  uint32_t max_frames,
                                  uint32_t max_polyphony) {
    if (!engine) return;

    memset(engine, 0, sizeof(ModalEffectEngine));

    // Create C++ engine and event queue (only allocation happens here, not in render!)
    engine->synth_engine = new SynthEngine(max_polyphony);
    engine->event_queue = new EventQueue();

    // Allocate wet signal buffers
    engine->buffer_size = max_frames;
    engine->wetL = new float[max_frames];
    engine->wetR = new float[max_frames];
    memset(engine->wetL, 0, max_frames * sizeof(float));
    memset(engine->wetR, 0, max_frames * sizeof(float));

    // Prepare engine for processing
    engine->synth_engine->prepare(sample_rate, max_frames, 2);

    engine->initialized = true;
}

void modal_attractors_engine_prepare(ModalEffectEngine* engine,
                                     double sample_rate,
                                     uint32_t max_frames) {
    if (!engine || !engine->initialized) return;

    // Reallocate buffers if size changed
    if (max_frames > engine->buffer_size) {
        delete[] engine->wetL;
        delete[] engine->wetR;
        engine->buffer_size = max_frames;
        engine->wetL = new float[max_frames];
        engine->wetR = new float[max_frames];
        memset(engine->wetL, 0, max_frames * sizeof(float));
        memset(engine->wetR, 0, max_frames * sizeof(float));
    }

    engine->synth_engine->prepare(sample_rate, max_frames, 2);
}

void modal_attractors_engine_reset(ModalEffectEngine* engine) {
    if (!engine || !engine->initialized) return;

    engine->synth_engine->reset();
}

void modal_attractors_engine_cleanup(ModalEffectEngine* engine) {
    if (!engine) return;

    if (engine->synth_engine) {
        delete engine->synth_engine;
        engine->synth_engine = nullptr;
    }

    if (engine->event_queue) {
        delete engine->event_queue;
        engine->event_queue = nullptr;
    }

    if (engine->wetL) {
        delete[] engine->wetL;
        engine->wetL = nullptr;
    }

    if (engine->wetR) {
        delete[] engine->wetR;
        engine->wetR = nullptr;
    }

    engine->buffer_size = 0;
    engine->initialized = false;
}

// ============================================================================
// Event handling (real-time safe)
// ============================================================================

void modal_attractors_engine_begin_events(ModalEffectEngine* engine) {
    if (!engine || !engine->initialized) return;

    // Clear event queue for this render frame
    engine->event_queue->clear();
}

void modal_attractors_engine_push_note_on(ModalEffectEngine* engine,
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

void modal_attractors_engine_push_note_off(ModalEffectEngine* engine,
                                           int32_t sample_offset,
                                           uint8_t note) {
    if (!engine || !engine->initialized) return;

    SynthEvent event;
    event.type = EventType::NoteOff;
    event.sampleOffset = sample_offset;
    event.noteOff.note = note;

    engine->event_queue->push(event);
}

void modal_attractors_engine_push_pitch_bend(ModalEffectEngine* engine,
                                             int32_t sample_offset,
                                             float value) {
    if (!engine || !engine->initialized) return;

    SynthEvent event;
    event.type = EventType::PitchBend;
    event.sampleOffset = sample_offset;
    event.pitchBend.value = value;

    engine->event_queue->push(event);
}

void modal_attractors_engine_push_parameter(ModalEffectEngine* engine,
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

void modal_attractors_engine_render(ModalEffectEngine* engine,
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

void modal_attractors_engine_process(ModalEffectEngine* engine,
                                     const float* inL,
                                     const float* inR,
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

    // Get effect parameters
    float excite = engine->synth_engine->getParameter(2);  // kParam_Excite = 2
    float mix = engine->synth_engine->getParameter(4);     // kParam_Mix = 4
    float dryGain = 1.0f - mix;
    float wetGain = mix;

    // Calculate input energy (simple RMS over buffer)
    float energy = 0.0f;
    for (uint32_t i = 0; i < num_frames; ++i) {
        float sample = (inL[i] + inR[i]) * 0.5f;
        energy += sample * sample;
    }
    energy = sqrtf(energy / num_frames);

    // If input energy is above threshold, trigger a note
    // This is a simple approach - ideally we'd use continuous excitation
    static float lastEnergy = 0.0f;
    float energyDelta = energy - lastEnergy;
    lastEnergy = energy;

    if (energyDelta > 0.01f * excite && energy > 0.001f) {
        // Trigger note C4 (MIDI 60) with velocity based on energy
        float velocity = fminf(energy * 10.0f * excite, 1.0f);
        SynthEvent event;
        event.type = EventType::NoteOn;
        event.sampleOffset = 0;
        event.noteOn.note = 60;
        event.noteOn.velocity = velocity;
        event.noteOn.channel = 0;
        engine->event_queue->push(event);
    }

    // Render modal synthesis (wet signal) using pre-allocated buffers
    engine->synth_engine->render(*engine->event_queue, engine->wetL, engine->wetR, num_frames);

    // Mix dry and wet signals
    for (uint32_t i = 0; i < num_frames; ++i) {
        outL[i] = inL[i] * dryGain + engine->wetL[i] * wetGain;
        outR[i] = inR[i] * dryGain + engine->wetR[i] * wetGain;
    }
}

// ============================================================================
// Parameter access
// ============================================================================

void modal_attractors_engine_set_parameter(ModalEffectEngine* engine,
                                           uint32_t param_id,
                                           float value) {
    if (!engine || !engine->initialized) return;

    engine->synth_engine->setParameter(param_id, value);
}

float modal_attractors_engine_get_parameter(ModalEffectEngine* engine,
                                            uint32_t param_id) {
    if (!engine || !engine->initialized) return 0.0f;

    return engine->synth_engine->getParameter(param_id);
}
