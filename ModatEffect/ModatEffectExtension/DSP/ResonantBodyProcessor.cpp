/**
 * @file ResonantBodyProcessor.cpp
 * @brief Main resonant body effect processor implementation
 */

#include "ResonantBodyProcessor.h"
#include "audio_synth.h"
#include <math.h>
#include <string.h>

// Default base frequencies for each band (low, mid, high)
static const float DEFAULT_BASE_FREQS[MAX_RESONATORS] = {
    150.0f,   // Low band base frequency
    600.0f,   // Mid band base frequency
    2400.0f   // High band base frequency
};

// Helper function to map body size to frequency multiplier
static float body_size_to_freq_mult(float body_size) {
    // body_size: 0 = small (high pitch), 1 = large (low pitch)
    // Frequency multiplier range: 0.5 to 2.0
    return 2.0f - (body_size * 1.5f);
}

// Helper function to map material to damping
static float material_to_damping(float material) {
    // material: 0 = soft (high damping, short decay), 1 = hard (low damping, long ring)
    // Damping range: 0.5 to 50.0 (higher = faster decay)
    return 50.0f - (material * 49.5f);
}

// Helper function to configure resonator modes
static void configure_resonator_modes(modal_node_t* resonator,
                                     float base_freq,
                                     float freq_mult,
                                     float damping) {
    // Configure 4 modes with harmonic relationships
    float freqs[4] = {
        base_freq * freq_mult * 1.0f,   // Fundamental
        base_freq * freq_mult * 2.3f,   // Slightly inharmonic 2nd mode
        base_freq * freq_mult * 3.7f,   // Slightly inharmonic 3rd mode
        base_freq * freq_mult * 5.2f    // Slightly inharmonic 4th mode
    };

    float weights[4] = {1.0f, 0.6f, 0.3f, 0.15f}; // Decreasing weights

    for (int i = 0; i < 4; i++) {
        float omega = freq_to_omega(freqs[i]);
        modal_node_set_mode(resonator, i, omega, damping, weights[i]);
        resonator->modes[i].params.shape = WAVE_SHAPE_SINE;
        resonator->modes[i].params.active = true;
    }
}

void resonant_body_init(resonant_body_processor_t* processor, float sample_rate) {
    if (!processor) return;

    processor->sample_rate = sample_rate;

    // Initialize DSP components
    energy_extractor_init(&processor->energy_extractor, sample_rate, 5.0f, 100.0f, 10.0f);
    spectral_analyzer_init(&processor->spectral_analyzer, sample_rate, 300.0f, 3000.0f);
    pitch_detector_init(&processor->pitch_detector, sample_rate, 60.0f, 2000.0f, 50.0f, 100.0f);

    // Initialize resonators (one per frequency band)
    for (int i = 0; i < MAX_RESONATORS; i++) {
        modal_node_init(&processor->resonators[i], i, PERSONALITY_RESONATOR);
        processor->base_freqs[i] = DEFAULT_BASE_FREQS[i];
    }

    // Initialize parameters
    processor->params.body_size = 0.5f;
    processor->params.material = 0.5f;
    processor->params.excite = 0.5f;
    processor->params.morph = 0.0f;
    processor->params.mix = 0.5f;

    // Configure initial resonator settings
    float freq_mult = body_size_to_freq_mult(processor->params.body_size);
    float damping = material_to_damping(processor->params.material);

    for (int i = 0; i < MAX_RESONATORS; i++) {
        configure_resonator_modes(&processor->resonators[i],
                                 processor->base_freqs[i],
                                 freq_mult,
                                 damping);
        processor->resonators[i].audio_gain = 0.3f;
        processor->resonators[i].carrier_freq_hz = processor->base_freqs[i];
        modal_node_start(&processor->resonators[i]);
    }

    // Control rate: update every ~200 samples at 48kHz (~240Hz control rate)
    processor->control_rate_divisor = (uint32_t)(sample_rate / 240.0f);
    processor->control_counter = 0;

    processor->initialized = true;
}

float resonant_body_process(resonant_body_processor_t* processor, float input) {
    if (!processor || !processor->initialized) return input;

    // 1. Extract energy envelope from input
    float energy = energy_extractor_process(&processor->energy_extractor, input);

    // 2. Split input into frequency bands
    float band_outputs[NUM_BANDS];
    spectral_analyzer_process(&processor->spectral_analyzer, input, band_outputs);

    // 3. Update pitch detector
    pitch_detector_process(&processor->pitch_detector, input);

    // 4. Control rate updates
    processor->control_counter++;
    if (processor->control_counter >= processor->control_rate_divisor) {
        processor->control_counter = 0;

        // Analyze pitch
        pitch_detector_analyze(&processor->pitch_detector);

        // Update resonator frequencies based on pitch tracking (morph parameter)
        if (processor->params.morph > 0.01f && pitch_detector_is_valid(&processor->pitch_detector)) {
            float detected_pitch = pitch_detector_get_smoothed_pitch(&processor->pitch_detector);
            float freq_mult = body_size_to_freq_mult(processor->params.body_size);

            // Blend between fixed and tracked frequencies
            for (int i = 0; i < MAX_RESONATORS; i++) {
                float fixed_freq = processor->base_freqs[i] * freq_mult;
                float tracked_freq = detected_pitch * (i + 1); // Harmonics of detected pitch
                float final_freq = fixed_freq * (1.0f - processor->params.morph) +
                                  tracked_freq * processor->params.morph;

                processor->resonators[i].carrier_freq_hz = final_freq;

                // Update mode frequencies
                float damping = material_to_damping(processor->params.material);
                configure_resonator_modes(&processor->resonators[i], final_freq, 1.0f, damping);
            }
        }

        // Update modal nodes
        for (int i = 0; i < MAX_RESONATORS; i++) {
            modal_node_step(&processor->resonators[i]);
        }
    }

    // 5. Excite resonators with band-filtered signals scaled by energy and excite parameter
    float excitation_scale = energy * processor->params.excite;

    for (int i = 0; i < MAX_RESONATORS; i++) {
        // Create poke event from band output
        if (fabsf(band_outputs[i]) > 0.001f) {
            poke_event_t poke;
            poke.source_node_id = 0;
            poke.strength = fabsf(band_outputs[i]) * excitation_scale * 10.0f; // Scale for audibility
            poke.phase_hint = (band_outputs[i] > 0.0f) ? 0.0f : M_PI;

            // Equal weight to all modes
            for (int m = 0; m < MAX_MODES; m++) {
                poke.mode_weights[m] = 0.25f;
            }

            modal_node_apply_poke(&processor->resonators[i], &poke);
        }
    }

    // 6. Render audio from resonators
    float wet_output = 0.0f;
    for (int i = 0; i < MAX_RESONATORS; i++) {
        // Get amplitude from modal node
        float amp = modal_node_get_amplitude(&processor->resonators[i]);

        // Simple synthesis: use carrier frequency
        float phase = (float)processor->resonators[i].step_count *
                     processor->resonators[i].carrier_freq_hz / processor->sample_rate;
        phase = fmodf(phase, 1.0f) * 2.0f * M_PI;

        wet_output += amp * sinf(phase) * processor->resonators[i].audio_gain;
    }

    // 7. Mix dry and wet signals
    float dry_wet = processor->params.mix;
    float output = input * (1.0f - dry_wet) + wet_output * dry_wet;

    return output;
}

void resonant_body_process_buffer(resonant_body_processor_t* processor,
                                  const float* input_L,
                                  const float* input_R,
                                  float* output_L,
                                  float* output_R,
                                  uint32_t num_samples) {
    if (!processor || !input_L || !input_R || !output_L || !output_R) return;

    for (uint32_t i = 0; i < num_samples; i++) {
        // Process as mono (sum L+R)
        float mono_input = (input_L[i] + input_R[i]) * 0.5f;
        float mono_output = resonant_body_process(processor, mono_input);

        // Output to both channels
        output_L[i] = mono_output;
        output_R[i] = mono_output;
    }
}

void resonant_body_set_body_size(resonant_body_processor_t* processor, float size) {
    if (!processor) return;
    processor->params.body_size = fmaxf(0.0f, fminf(1.0f, size));

    // Update resonator frequencies
    float freq_mult = body_size_to_freq_mult(processor->params.body_size);
    float damping = material_to_damping(processor->params.material);

    for (int i = 0; i < MAX_RESONATORS; i++) {
        configure_resonator_modes(&processor->resonators[i],
                                 processor->base_freqs[i],
                                 freq_mult,
                                 damping);
    }
}

void resonant_body_set_material(resonant_body_processor_t* processor, float material) {
    if (!processor) return;
    processor->params.material = fmaxf(0.0f, fminf(1.0f, material));

    // Update resonator damping
    float freq_mult = body_size_to_freq_mult(processor->params.body_size);
    float damping = material_to_damping(processor->params.material);

    for (int i = 0; i < MAX_RESONATORS; i++) {
        configure_resonator_modes(&processor->resonators[i],
                                 processor->base_freqs[i],
                                 freq_mult,
                                 damping);
    }
}

void resonant_body_set_excite(resonant_body_processor_t* processor, float excite) {
    if (!processor) return;
    processor->params.excite = fmaxf(0.0f, fminf(1.0f, excite));
}

void resonant_body_set_morph(resonant_body_processor_t* processor, float morph) {
    if (!processor) return;
    processor->params.morph = fmaxf(0.0f, fminf(1.0f, morph));
}

void resonant_body_set_mix(resonant_body_processor_t* processor, float mix) {
    if (!processor) return;
    processor->params.mix = fmaxf(0.0f, fminf(1.0f, mix));
}

void resonant_body_reset(resonant_body_processor_t* processor) {
    if (!processor) return;

    energy_extractor_reset(&processor->energy_extractor);
    spectral_analyzer_reset(&processor->spectral_analyzer);
    pitch_detector_reset(&processor->pitch_detector);

    for (int i = 0; i < MAX_RESONATORS; i++) {
        modal_node_reset(&processor->resonators[i]);
    }

    processor->control_counter = 0;
}

void resonant_body_cleanup(resonant_body_processor_t* processor) {
    if (!processor) return;

    energy_extractor_cleanup(&processor->energy_extractor);
    pitch_detector_cleanup(&processor->pitch_detector);

    processor->initialized = false;
}
