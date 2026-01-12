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

    // Initialize effect state
    engine->last_energy = 0.0f;
    engine->smoothed_energy = 0.0f;
    engine->current_note = 60;  // C4
    engine->note_is_on = false;
    engine->energy_threshold = 0.01f;
    engine->sample_rate = sample_rate;

    // Allocate pitch detection buffer (100ms window)
    engine->pitch_buf_size = static_cast<uint32_t>(sample_rate * 0.1);  // 100ms
    engine->pitch_buffer = new float[engine->pitch_buf_size];
    memset(engine->pitch_buffer, 0, engine->pitch_buf_size * sizeof(float));
    engine->pitch_buf_pos = 0;
    engine->detected_pitch_hz = 261.63f;  // C4 default

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

    // Reallocate pitch buffer if sample rate changed
    uint32_t new_pitch_buf_size = static_cast<uint32_t>(sample_rate * 0.1);  // 100ms
    if (sample_rate != engine->sample_rate) {
        delete[] engine->pitch_buffer;
        engine->pitch_buf_size = new_pitch_buf_size;
        engine->pitch_buffer = new float[engine->pitch_buf_size];
        memset(engine->pitch_buffer, 0, engine->pitch_buf_size * sizeof(float));
        engine->pitch_buf_pos = 0;
        engine->sample_rate = sample_rate;
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

    if (engine->pitch_buffer) {
        delete[] engine->pitch_buffer;
        engine->pitch_buffer = nullptr;
    }

    engine->buffer_size = 0;
    engine->pitch_buf_size = 0;
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

// Simple pitch detection using zero-crossing rate
static float detect_pitch_zcr(const float* buffer, uint32_t size, float sample_rate) {
    // Count zero crossings
    uint32_t crossings = 0;
    for (uint32_t i = 1; i < size; ++i) {
        if ((buffer[i-1] >= 0.0f && buffer[i] < 0.0f) ||
            (buffer[i-1] < 0.0f && buffer[i] >= 0.0f)) {
            crossings++;
        }
    }

    // Estimate frequency from zero-crossing rate
    // Each crossing is half a cycle
    float freq = (crossings * sample_rate) / (2.0f * size);

    // Constrain to reasonable range (60 Hz - 2000 Hz)
    if (freq < 60.0f) freq = 60.0f;
    if (freq > 2000.0f) freq = 2000.0f;

    return freq;
}

// Convert frequency in Hz to MIDI note number
static uint8_t hz_to_midi(float hz) {
    // MIDI note = 69 + 12 * log2(hz / 440)
    float note_f = 69.0f + 12.0f * log2f(hz / 440.0f);
    int note_i = static_cast<int>(note_f + 0.5f);  // Round
    if (note_i < 0) note_i = 0;
    if (note_i > 127) note_i = 127;
    return static_cast<uint8_t>(note_i);
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
    float bodySize = engine->synth_engine->getParameter(0);  // kParam_BodySize = 0
    float material = engine->synth_engine->getParameter(1);  // kParam_Material = 1
    float excite = engine->synth_engine->getParameter(2);    // kParam_Excite = 2
    float morph = engine->synth_engine->getParameter(3);     // kParam_Morph = 3
    float mix = engine->synth_engine->getParameter(4);       // kParam_Mix = 4

    float dryGain = 1.0f - mix;
    float wetGain = mix;

    // Calculate input energy (RMS over buffer)
    float energy = 0.0f;
    for (uint32_t i = 0; i < num_frames; ++i) {
        float sample = (inL[i] + inR[i]) * 0.5f;

        // Fill pitch detection buffer
        engine->pitch_buffer[engine->pitch_buf_pos] = sample;
        engine->pitch_buf_pos = (engine->pitch_buf_pos + 1) % engine->pitch_buf_size;

        energy += sample * sample;
    }
    energy = sqrtf(energy / num_frames);

    // Smooth energy for continuous excitation
    const float energy_smooth = 0.95f;
    engine->smoothed_energy = engine->smoothed_energy * energy_smooth + energy * (1.0f - energy_smooth);

    // Detect onsets (energy rising above threshold)
    float energyDelta = energy - engine->last_energy;
    engine->last_energy = energy;

    // Adaptive threshold based on smoothed energy
    float threshold = 0.005f + engine->smoothed_energy * 0.5f;

    // Detect pitch from input (every buffer)
    float detected_freq = detect_pitch_zcr(engine->pitch_buffer, engine->pitch_buf_size,
                                          static_cast<float>(engine->sample_rate));
    engine->detected_pitch_hz = detected_freq;

    // Calculate base note from bodySize parameter
    // bodySize: 0.0 = C2 (65.4 Hz), 0.5 = C4 (261.6 Hz), 1.0 = C6 (1046.5 Hz)
    // Map to MIDI notes: 36 (C2) to 96 (C6)
    float bodySize_note_f = 36.0f + bodySize * 60.0f;  // MIDI 36-96
    uint8_t base_note = static_cast<uint8_t>(bodySize_note_f);

    // If morph > 0, blend towards detected pitch
    uint8_t target_note = base_note;
    if (morph > 0.01f) {
        uint8_t detected_note = hz_to_midi(detected_freq);
        // Blend between bodySize base note and detected note based on morph parameter
        float blended_note = base_note * (1.0f - morph) + detected_note * morph;
        target_note = static_cast<uint8_t>(blended_note + 0.5f);
    }

    // Trigger note on rising energy (onset detection)
    if (energyDelta > threshold * excite && energy > 0.002f * excite) {
        // Send note-off for previous note if one is playing
        if (engine->note_is_on) {
            SynthEvent noteOff;
            noteOff.type = EventType::NoteOff;
            noteOff.sampleOffset = 0;
            noteOff.noteOff.note = engine->current_note;
            engine->event_queue->push(noteOff);
        }

        // Trigger new note with velocity based on energy and excite
        float velocity = fminf(energy * 20.0f * (0.5f + excite * 0.5f), 1.0f);
        if (velocity < 0.1f) velocity = 0.1f;

        SynthEvent noteOn;
        noteOn.type = EventType::NoteOn;
        noteOn.sampleOffset = 0;
        noteOn.noteOn.note = target_note;
        noteOn.noteOn.velocity = velocity;
        noteOn.noteOn.channel = 0;
        engine->event_queue->push(noteOn);

        engine->current_note = target_note;
        engine->note_is_on = true;
    }

    // Auto note-off when energy drops significantly (for cleaner sound)
    if (engine->note_is_on && engine->smoothed_energy < 0.001f) {
        SynthEvent noteOff;
        noteOff.type = EventType::NoteOff;
        noteOff.sampleOffset = 0;
        noteOff.noteOff.note = engine->current_note;
        engine->event_queue->push(noteOff);
        engine->note_is_on = false;
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
