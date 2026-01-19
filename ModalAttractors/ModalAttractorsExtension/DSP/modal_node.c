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
// Physical Mode Ratio Presets
// ============================================================================

/**
 * @brief Modal frequency preset definition
 *
 * Defines mode frequency ratios, damping ratios, and weights for physically-modeled objects.
 * Based on acoustics literature (Fletcher & Rossing, Rayleigh, etc.)
 */
typedef struct {
    float freq_ratio;    ///< Frequency ratio relative to fundamental (1.0 = fundamental)
    float damping_ratio; ///< Relative damping (1.0 = same as fundamental)
    float weight;        ///< Audio contribution weight [0,1]
} mode_preset_t;

typedef struct {
    const char* name;              ///< Preset name
    const char* description;       ///< Physical description
    mode_preset_t modes[MAX_MODES]; ///< 4 mode parameters
} modal_preset_t;

/**
 * @brief Physical mode presets based on real instruments and objects
 *
 * Sources:
 * - Fletcher & Rossing: "The Physics of Musical Instruments"
 * - Rayleigh: "Theory of Sound"
 * - Research papers on modal analysis
 */
static const modal_preset_t MODAL_PRESETS[] = {
    // CHURCH BELL (Western church bell partials)
    // Classic bell sound with characteristic "minor third" timbre
    {
        .name = "Church Bell",
        .description = "Western church bell with hum, fundamental, tierce, and quint",
        .modes = {
            {1.000f, 1.0f, 1.00f},  // Fundamental (strongest, longest)
            {1.190f, 1.2f, 0.75f},  // Tierce (minor third above, defines bell character)
            {1.500f, 1.4f, 0.60f},  // Quint (perfect fifth)
            {2.000f, 1.8f, 0.45f},  // Nominal (octave, decays faster)
        }
    },

    // CIRCULAR PLATE (Rayleigh modes)
    // Bright, metallic cymbal-like sound
    {
        .name = "Circular Plate",
        .description = "Flat circular plate (cymbal, gong) - Rayleigh modes",
        .modes = {
            {1.000f, 1.0f, 1.00f},  // (0,1) mode
            {2.081f, 1.1f, 0.70f},  // (1,1) mode
            {3.413f, 1.3f, 0.50f},  // (2,1) mode
            {3.891f, 1.4f, 0.35f},  // (0,2) mode
        }
    },

    // WINE GLASS (cylindrical shell modes)
    // Clear, ringing tone with harmonic overtones
    {
        .name = "Wine Glass",
        .description = "Cylindrical shell resonance (wine glass rim)",
        .modes = {
            {1.000f, 1.0f, 1.00f},  // Fundamental
            {2.280f, 1.3f, 0.65f},  // First overtone
            {3.650f, 1.6f, 0.45f},  // Second overtone
            {5.130f, 2.0f, 0.30f},  // Third overtone
        }
    },

    // FREE-FREE BAR (rectangular bar, both ends free)
    // Warm, mellow tone with widely-spaced partials
    {
        .name = "Free Bar",
        .description = "Rectangular bar with free ends (chime, vibraphone)",
        .modes = {
            {1.000f, 1.0f, 1.00f},  // Fundamental
            {2.756f, 1.2f, 0.60f},  // Second mode
            {5.404f, 1.5f, 0.35f},  // Third mode
            {8.933f, 1.8f, 0.20f},  // Fourth mode
        }
    },

    // TUNED BAR (marimba-style with arch tuning)
    // Musical harmonic series for melodic instruments
    {
        .name = "Tuned Bar",
        .description = "Arch-tuned bar (marimba, xylophone) with harmonic overtones",
        .modes = {
            {1.000f, 1.0f, 1.00f},  // Fundamental
            {4.000f, 1.4f, 0.50f},  // 2 octaves (tuned)
            {10.00f, 2.0f, 0.25f},  // ~3 octaves + fifth
            {18.00f, 2.5f, 0.15f},  // ~4 octaves
        }
    },

    // CIRCULAR DRUM (kettledrum/timpani modes)
    // Low, resonant with closely-spaced overtones
    {
        .name = "Drum Membrane",
        .description = "Circular membrane (kettledrum, timpani)",
        .modes = {
            {1.000f, 0.8f, 1.00f},  // (0,1) fundamental
            {1.593f, 1.0f, 0.70f},  // (1,1) mode
            {2.136f, 1.2f, 0.50f},  // (2,1) mode
            {2.296f, 1.3f, 0.40f},  // (0,2) mode
        }
    },

    // SMALL BELL (handbell, bicycle bell)
    // Bright, high-pitched with fast attack
    {
        .name = "Small Bell",
        .description = "Small handbell or bicycle bell (higher inharmonicity)",
        .modes = {
            {1.000f, 0.6f, 1.00f},  // Fundamental (shorter decay)
            {1.350f, 0.8f, 0.80f},  // Stretched minor third
            {1.700f, 1.0f, 0.60f},  // Stretched fifth
            {2.200f, 1.2f, 0.40f},  // Upper partial
        }
    },

    // HARMONIC (string-like)
    // Perfect harmonic series for comparison
    {
        .name = "Harmonic String",
        .description = "Idealized string with perfect harmonic overtones",
        .modes = {
            {1.000f, 1.0f, 1.00f},  // Fundamental
            {2.000f, 1.1f, 0.60f},  // Octave
            {3.000f, 1.2f, 0.40f},  // Fifth above octave
            {4.000f, 1.3f, 0.30f},  // Two octaves
        }
    },
};

#define NUM_MODAL_PRESETS (sizeof(MODAL_PRESETS) / sizeof(modal_preset_t))

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
        node->modes[k].params.shape = WAVE_SHAPE_SINE;  // Default to sine wave
    }

    node->coupling_strength = 0.3f;
    node->global_damping = 0.0f;  // No extra damping by default
    node->carrier_freq_hz = 440.0f;
    node->audio_gain = 0.7f;
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

            float strength = node->excitation.strength * mode->params.weight;
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
    for (int k = 0; k < MAX_MODES; k++) {
        if (!node->modes[k].params.active) continue;

        float weight = poke->mode_weights[k];
        float phase = (poke->phase_hint < 0.0f) ? random_phase() : poke->phase_hint;

        // Small immediate kick
        float kick_strength = poke->strength * weight * 0.1f;
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

// ============================================================================
// Physical Preset API
// ============================================================================

uint8_t modal_node_get_num_presets(void) {
    return (uint8_t)NUM_MODAL_PRESETS;
}

const char* modal_node_get_preset_name(uint8_t preset_idx) {
    if (preset_idx >= NUM_MODAL_PRESETS) return NULL;
    return MODAL_PRESETS[preset_idx].name;
}

const char* modal_node_get_preset_description(uint8_t preset_idx) {
    if (preset_idx >= NUM_MODAL_PRESETS) return NULL;
    return MODAL_PRESETS[preset_idx].description;
}

void modal_node_apply_preset(modal_node_t* node, uint8_t preset_idx,
                             float fundamental_freq_hz, float base_damping) {
    if (preset_idx >= NUM_MODAL_PRESETS) return;

    const modal_preset_t* preset = &MODAL_PRESETS[preset_idx];
    float fundamental_omega = freq_to_omega(fundamental_freq_hz);

    // Apply preset to all 4 modes
    for (int k = 0; k < MAX_MODES; k++) {
        const mode_preset_t* mode_preset = &preset->modes[k];

        // Calculate omega from frequency ratio
        float omega = fundamental_omega * mode_preset->freq_ratio;

        // Calculate damping from base damping and damping ratio
        float gamma = base_damping * mode_preset->damping_ratio;

        // Set mode with preset parameters
        modal_node_set_mode(node, k, omega, gamma, mode_preset->weight);
    }
}
