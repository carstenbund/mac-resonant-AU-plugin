/**
 * @file modal_node.h
 * @brief Core 4-mode modal resonator node implementation (macOS port)
 *
 * This module implements the autonomous modal resonator with up to 4 complex modes.
 * Each mode evolves according to: ȧ_k = (-γ_k + iω_k)a_k + u_k(t)
 *
 * Ported from ESP32 firmware for macOS Audio Unit plugin:
 * - Removed FreeRTOS dependencies
 * - Adapted for variable sample rates
 * - Ready for C++ wrapper integration
 */

#ifndef MODAL_NODE_H
#define MODAL_NODE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <complex>
// Internal C++ complex type
typedef std::complex<float> modal_complex_t;
extern "C" {
#else
#include <complex.h>
// C99 complex type
typedef float complex modal_complex_t;
#endif

// C-compatible complex float (for C ABI / Swift bridging)
typedef struct {
    float re;
    float im;
} modal_complexf_t;


// ============================================================================
// Constants
// ============================================================================

#define MAX_MODES 4
#define MAX_NEIGHBORS 8
#define CONTROL_RATE_HZ 500  // 500 Hz control rate (2ms timestep)
#define CONTROL_DT (1.0f / CONTROL_RATE_HZ)

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief Node personality types
 */
typedef enum {
    PERSONALITY_RESONATOR,      ///< Decays to silence (percussive)
    PERSONALITY_SELF_OSCILLATOR ///< Continuous sound (drone)
} node_personality_t;

/**
 * @brief Oscillator wave shapes
 */
typedef enum {
    WAVE_SHAPE_SINE = 0,      ///< Pure sine wave (default)
    WAVE_SHAPE_SAWTOOTH,      ///< Sawtooth (all harmonics, 1/n amplitude)
    WAVE_SHAPE_TRIANGLE,      ///< Triangle (odd harmonics, 1/n² amplitude)
    WAVE_SHAPE_SQUARE,        ///< Square wave (odd harmonics, 1/n amplitude)
    WAVE_SHAPE_PULSE_25,      ///< Pulse wave, 25% duty cycle
    WAVE_SHAPE_PULSE_10,      ///< Pulse wave, 10% duty cycle (thin)
    WAVE_SHAPE_COUNT          ///< Number of wave shapes
} wave_shape_t;

/**
 * @brief Single mode parameters
 */
typedef struct {
    float omega;          ///< Angular frequency (rad/s)
    float gamma;          ///< Damping coefficient (>0 for stability)
    float weight;         ///< Audio contribution weight [0,1]
    wave_shape_t shape;   ///< Oscillator wave shape for this mode
    bool active;          ///< Mode enabled flag
} mode_params_t;

/**
 * @brief Modal state (complex amplitude and dynamics)
 */
typedef struct {
    modal_complex_t a;        ///< Complex amplitude a(t) = |a|e^(iφ)
    modal_complex_t a_dot;    ///< Time derivative (for integration)
    mode_params_t params;   ///< Mode parameters
} mode_state_t;

/**
 * @brief Excitation envelope (for poke events)
 */
typedef struct {
    float strength;         ///< Excitation strength
    float duration_ms;      ///< Envelope duration (1-20ms)
    float elapsed_ms;       ///< Time since poke start
    float phase_hint;       ///< Optional phase hint (radians)
    bool active;            ///< Envelope active flag
} excitation_envelope_t;

/**
 * @brief Node state (4 modes + metadata)
 */
typedef struct {
    uint8_t node_id;                    ///< Unique node identifier
    node_personality_t personality;      ///< Resonator or self-oscillator

    mode_state_t modes[MAX_MODES];      ///< 4 complex modes
    excitation_envelope_t excitation;   ///< Current excitation envelope

    float coupling_strength;            ///< Global coupling coefficient
    float global_damping;               ///< Global damping coefficient (added to all modes)
    uint8_t num_neighbors;              ///< Number of connected neighbors
    uint8_t neighbor_ids[MAX_NEIGHBORS];///< Neighbor node IDs

    float carrier_freq_hz;              ///< Base audio frequency (Hz)
    float audio_gain;                   ///< Master output gain [0,1]

    uint32_t step_count;                ///< Simulation step counter
    bool running;                       ///< Node running flag
} modal_node_t;

/**
 * @brief Poke event (network excitation)
 */
typedef struct {
    uint8_t source_node_id;   ///< Sending node ID
    float strength;           ///< Excitation strength
    float phase_hint;         ///< Phase hint (radians, or -1 for random)
    float mode_weights[MAX_MODES]; ///< Per-mode weighting
} poke_event_t;

// ============================================================================
// Core API
// ============================================================================

/**
 * @brief Initialize modal node with default parameters
 *
 * @param node Pointer to node structure
 * @param node_id Unique node identifier
 * @param personality Node personality type
 */
void modal_node_init(modal_node_t* node, uint8_t node_id, node_personality_t personality);

/**
 * @brief Configure a single mode
 *
 * @param node Pointer to node structure
 * @param mode_idx Mode index [0..MAX_MODES-1]
 * @param omega Angular frequency (rad/s)
 * @param gamma Damping coefficient
 * @param weight Audio contribution weight [0,1]
 */
void modal_node_set_mode(modal_node_t* node, uint8_t mode_idx,
                         float omega, float gamma, float weight);

/**
 * @brief Set node neighbors for coupling
 *
 * @param node Pointer to node structure
 * @param neighbor_ids Array of neighbor node IDs
 * @param num_neighbors Number of neighbors
 */
void modal_node_set_neighbors(modal_node_t* node,
                              uint8_t* neighbor_ids,
                              uint8_t num_neighbors);

/**
 * @brief Simulate one timestep (call at CONTROL_RATE_HZ)
 *
 * This function integrates the modal dynamics for one timestep using
 * exact exponential integration for numerical stability.
 *
 * @param node Pointer to node structure
 */
void modal_node_step(modal_node_t* node);

/**
 * @brief Apply poke excitation to node
 *
 * Excitation is applied via a short envelope (1-20ms) to all modes
 * according to mode_weights.
 *
 * @param node Pointer to node structure
 * @param poke Poke event data
 */
void modal_node_apply_poke(modal_node_t* node, const poke_event_t* poke);

/**
 * @brief Get current audio amplitude (for synthesis)
 *
 * Combines all mode amplitudes with their weights.
 *
 * @param node Pointer to node structure
 * @return Instantaneous amplitude [0,1]
 */
float modal_node_get_amplitude(const modal_node_t* node);

/**
 * @brief Get phase modulation (from mode 2)
 *
 * @param node Pointer to node structure
 * @return Phase modulation in radians
 */
float modal_node_get_phase_modulation(const modal_node_t* node);

/**
 * @brief Get mode 0 complex amplitude (for network broadcast)
 *
 * @param node Pointer to node structure
 * @return Complex amplitude of mode 0
 */
modal_complexf_t modal_node_get_mode0(const modal_node_t* node);

/**
 * @brief Start node operation
 *
 * @param node Pointer to node structure
 */
void modal_node_start(modal_node_t* node);

/**
 * @brief Stop node operation
 *
 * @param node Pointer to node structure
 */
void modal_node_stop(modal_node_t* node);

/**
 * @brief Reset node state (clear all modes)
 *
 * @param node Pointer to node structure
 */
void modal_node_reset(modal_node_t* node);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Convert MIDI note to frequency (Hz)
 *
 * @param note MIDI note number [0..127]
 * @return Frequency in Hz
 */
float midi_to_freq(uint8_t note);

/**
 * @brief Convert frequency to angular frequency (rad/s)
 *
 * @param freq_hz Frequency in Hz
 * @return Angular frequency in rad/s
 */
float freq_to_omega(float freq_hz);

/**
 * @brief Generate random phase in [0, 2π)
 *
 * @return Random phase in radians
 */
float random_phase(void);

#ifdef __cplusplus
}

// C++ convenience wrapper (not C linkage)
inline modal_complex_t modal_node_get_mode0_cpp(const modal_node_t* node) {
    modal_complexf_t z = modal_node_get_mode0(node);
    return modal_complex_t(z.re, z.im);
}
#endif

#endif // MODAL_NODE_H
