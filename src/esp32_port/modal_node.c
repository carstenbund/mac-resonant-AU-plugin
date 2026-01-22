/**
 * @file modal_node.c
 * @brief Core 4-mode modal resonator implementation (macOS port)
 *
 * Implements autonomous modal oscillator with up to 4 complex modes.
 * Ported from ESP32 firmware - FreeRTOS dependencies removed.
 *
 * Dynamics: ȧ_k = (-γ_k + iω_k)a_k + u_k(t)
 */

#include "modal_node.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// Constants
// ============================================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MIDI_A4 69
#define FREQ_A4 440.0f

// ============================================================================
// Utility Functions
// ============================================================================

float midi_to_freq(uint8_t note) {
    // Standard MIDI to frequency: f = 440 * 2^((n-69)/12)
    return FREQ_A4 * powf(2.0f, (note - MIDI_A4) / 12.0f);
}

float freq_to_omega(float freq_hz) {
    return 2.0f * M_PI * freq_hz;
}

float random_phase(void) {
    // Simple random phase [0, 2π)
    return ((float)rand() / RAND_MAX) * 2.0f * M_PI;
}

float waveform_harmonic_amplitude(wave_shape_t waveform, int harmonic_idx) {
    // harmonic_idx: 0 = fundamental, 1 = 2nd harmonic, 2 = 3rd harmonic, etc.
    int n = harmonic_idx + 1; // harmonic number (1, 2, 3, ...)

    switch (waveform) {
        case WAVE_SHAPE_SINE:
            // Pure fundamental only
            return (harmonic_idx == 0) ? 1.0f : 0.0f;

        case WAVE_SHAPE_SAWTOOTH:
            // All harmonics: amplitude = 1/n
            return 1.0f / (float)n;

        case WAVE_SHAPE_SQUARE:
            // Odd harmonics only: amplitude = 1/n
            return (n % 2 == 1) ? (1.0f / (float)n) : 0.0f;

        case WAVE_SHAPE_TRIANGLE:
            // Odd harmonics only: amplitude = 1/n²
            if (n % 2 == 0) return 0.0f;
            return 1.0f / (float)(n * n);

        case WAVE_SHAPE_PULSE_25:
            // Pulse wave (25% duty): complex harmonic pattern
            // For simplicity: all harmonics with sin(πn/4)/(πn/4) envelope
            {
                float duty = 0.25f;
                float sinc = sinf(M_PI * n * duty) / (M_PI * n * duty);
                return fabsf(sinc) * (1.0f / (float)n);
            }

        case WAVE_SHAPE_PULSE_10:
            // Pulse wave (10% duty): narrower pulse, more harmonics
            {
                float duty = 0.10f;
                float sinc = sinf(M_PI * n * duty) / (M_PI * n * duty);
                return fabsf(sinc) * (1.0f / (float)n);
            }

        default:
            // Fallback to sine
            return (harmonic_idx == 0) ? 1.0f : 0.0f;
    }
}

// ============================================================================
// Complex Math Helpers
// ============================================================================

/**
 * @brief Complex multiply: c = a * b
 */
static inline float complex cmul(float complex a, float complex b) {
    return a * b;
}

/**
 * @brief Complex exponential: exp(i*theta)
 */
static inline float complex cexp_i(float theta) {
    return cosf(theta) + I * sinf(theta);
}

/**
 * @brief Complex magnitude: |z|
 */
static inline float cabs_f(float complex z) {
    return cabsf(z);
}

/**
 * @brief Complex argument (phase): arg(z)
 */
static inline float carg_f(float complex z) {
    return cargf(z);
}

// ============================================================================
// Modal Node Core
// ============================================================================

void modal_node_init(modal_node_t* node, uint8_t node_id, node_personality_t personality) {
    memset(node, 0, sizeof(modal_node_t));

    node->node_id = node_id;
    node->personality = personality;

    // Initialize all modes to small noise
    for (int k = 0; k < MAX_MODES; k++) {
        float real = ((float)rand() / RAND_MAX - 0.5f) * 0.01f;
        float imag = ((float)rand() / RAND_MAX - 0.5f) * 0.01f;
        node->modes[k].a = real + I * imag;
        node->modes[k].a_dot = 0.0f;
        node->modes[k].params.active = false;
        node->modes[k].params.shape = WAVE_SHAPE_SINE;  // Legacy parameter (now unused)
    }

    node->coupling_strength = 0.3f;
    node->global_damping = 0.0f;  // No extra damping by default
    node->carrier_freq_hz = 440.0f;
    node->audio_gain = 0.7f;
    node->excitation_waveform = WAVE_SHAPE_SINE;  // Default: pure sine excitation (fundamental only)
    node->running = false;
    node->step_count = 0;
}

void modal_node_set_mode(modal_node_t* node, uint8_t mode_idx,
                         float omega, float gamma, float weight) {
    if (mode_idx >= MAX_MODES) return;

    mode_state_t* mode = &node->modes[mode_idx];
    mode->params.omega = omega;
    mode->params.gamma = gamma;
    mode->params.weight = weight;
    mode->params.active = true;
    // Note: shape is not set here - preserves existing shape or uses default from init
}

void modal_node_set_neighbors(modal_node_t* node,
                              uint8_t* neighbor_ids,
                              uint8_t num_neighbors) {
    node->num_neighbors = (num_neighbors > MAX_NEIGHBORS) ? MAX_NEIGHBORS : num_neighbors;
    memcpy(node->neighbor_ids, neighbor_ids, node->num_neighbors);
}

void modal_node_step(modal_node_t* node) {
    if (!node->running) return;

    // Update excitation envelope if active
    if (node->excitation.active) {
        node->excitation.elapsed_ms += CONTROL_DT * 1000.0f;

        if (node->excitation.elapsed_ms >= node->excitation.duration_ms) {
            node->excitation.active = false;
        }
    }

    // Integrate each mode
    for (int k = 0; k < MAX_MODES; k++) {
        if (!node->modes[k].params.active) continue;

        mode_state_t* mode = &node->modes[k];
        float omega = mode->params.omega;
        float gamma = mode->params.gamma;

        // Personality-specific dynamics
        float effective_gamma = gamma;
        if (node->personality == PERSONALITY_SELF_OSCILLATOR) {
            // Self-oscillator: negative damping at low energy, positive at high
            float energy = cabsf(mode->a);
            float saturation_level = 1.0f;

            // Van der Pol-like: γ_eff = -γ + β*|a|²
            effective_gamma = -gamma + 3.0f * gamma * (energy * energy) / (saturation_level * saturation_level);
        }

        // Apply global damping (circuit energy control)
        // This adds extra damping to all modes, allowing manual system calming
        effective_gamma += node->global_damping;

        // Linear dynamics: ȧ = (-γ + iω)a
        float complex linear_term = (-effective_gamma + I * omega) * mode->a;

        // Excitation term (if envelope active)
        float complex excitation_term = 0.0f;
        if (node->excitation.active) {
            // Envelope shape: Hann window
            float t_norm = node->excitation.elapsed_ms / node->excitation.duration_ms;
            float envelope = 0.5f * (1.0f - cosf(M_PI * t_norm));

            // Excitation with phase hint
            float phase = node->excitation.phase_hint;
            if (phase < 0.0f) {
                phase = random_phase();
            }

            // Apply waveform-based spectral shaping
            // This distributes excitation energy according to harmonic content
            float harmonic_amplitude = waveform_harmonic_amplitude(node->excitation_waveform, k);

            float strength = node->excitation.strength * mode->params.weight * harmonic_amplitude;
            excitation_term = strength * envelope * cexp_i(phase);
        }

        // Total derivative
        mode->a_dot = linear_term + excitation_term;

        // Exact exponential integration for linear part (more stable than Euler)
        // For ȧ = λa, exact solution over dt: a(t+dt) = a(t) * exp(λ*dt)
        // We approximate: a_new ≈ a * exp(λ*dt) + excitation_contribution

        float complex lambda = -effective_gamma + I * omega;
        float complex exp_lambda_dt = cexpf(lambda * CONTROL_DT);

        // Update: exact for linear + simple addition for excitation
        mode->a = mode->a * exp_lambda_dt + excitation_term * CONTROL_DT;
    }

    node->step_count++;
}

void modal_node_apply_poke(modal_node_t* node, const poke_event_t* poke) {
    // Set up excitation envelope
    node->excitation.strength = poke->strength;
    node->excitation.phase_hint = poke->phase_hint;
    node->excitation.duration_ms = 10.0f; // Default 10ms envelope
    node->excitation.elapsed_ms = 0.0f;
    node->excitation.active = true;

    // For immediate effect, also add a small kick to active modes
    // Apply waveform-based spectral shaping to preserve physical model
    for (int k = 0; k < MAX_MODES; k++) {
        if (!node->modes[k].params.active) continue;

        // Get base weight from poke event
        float base_weight = poke->mode_weights[k];

        // Apply harmonic amplitude based on excitation waveform
        // This is where "waveform selection" happens - via spectral shaping of drive
        float harmonic_amplitude = waveform_harmonic_amplitude(node->excitation_waveform, k);

        // Combined weight: poke weight × waveform spectral shape
        float effective_weight = base_weight * harmonic_amplitude;

        float phase = (poke->phase_hint < 0.0f) ? random_phase() : poke->phase_hint;

        // Small immediate kick with spectral shaping
        float kick_strength = poke->strength * effective_weight * 0.1f;
        node->modes[k].a += kick_strength * cexp_i(phase);
    }
}

float modal_node_get_amplitude(const modal_node_t* node) {
    // Combine all mode amplitudes with weights
    float total = 0.0f;

    for (int k = 0; k < MAX_MODES; k++) {
        if (!node->modes[k].params.active) continue;

        float amp = cabsf(node->modes[k].a);
        float weight = node->modes[k].params.weight;
        total += amp * weight;
    }

    // Normalize to [0, 1] range (assuming max ~2.0 for 4 modes)
    return fminf(total / 2.0f, 1.0f);
}

float modal_node_get_phase_modulation(const modal_node_t* node) {
    // Use mode 2 phase for timbre modulation
    if (!node->modes[2].params.active) return 0.0f;

    // Return phase scaled by amplitude
    float amp = cabsf(node->modes[2].a);
    float phase = cargf(node->modes[2].a);

    return phase * amp * 0.1f; // Scale to reasonable range
}

modal_complexf_t modal_node_get_mode0(const modal_node_t* node) {
    float complex z = node->modes[0].a;
    modal_complexf_t result = { .re = crealf(z), .im = cimagf(z) };
    return result;
}

void modal_node_start(modal_node_t* node) {
    node->running = true;
}

void modal_node_stop(modal_node_t* node) {
    node->running = false;
}

void modal_node_reset(modal_node_t* node) {
    for (int k = 0; k < MAX_MODES; k++) {
        node->modes[k].a = 0.0f;
        node->modes[k].a_dot = 0.0f;
    }
    node->excitation.active = false;
    node->step_count = 0;
}
