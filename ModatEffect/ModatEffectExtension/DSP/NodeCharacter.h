/**
 * @file NodeCharacter.h
 * @brief Node character definitions for 5-node network system
 *
 * A character is a preset bundle that defines all modal parameters for a node.
 * Characters give nodes distinct sonic identities rather than being identical voices.
 */

#ifndef NODE_CHARACTER_H
#define NODE_CHARACTER_H

#include <cstdint>
#include "modal_node.h"

/**
 * @brief Complete character definition for a single node
 *
 * Encapsulates all parameters that define a node's sonic behavior:
 * - Modal frequency ratios (4 modes)
 * - Modal damping coefficients (4 modes)
 * - Modal audio weights (4 modes)
 * - Modal wave shapes (4 modes)
 * - Personality (resonator vs self-oscillator)
 * - Excitation behavior (poke strength/duration)
 * - Network coupling response
 */
struct NodeCharacter {
    // Mode parameters (4 modes per node)
    float mode_freq_mult[4];    ///< Frequency multipliers relative to base note
    float mode_damping[4];      ///< Damping coefficients (higher = faster decay)
    float mode_weight[4];       ///< Audio contribution weights (0.0-1.0)
    wave_shape_t mode_shape[4]; ///< Wave shapes for each mode

    // Voice behavior
    node_personality_t personality;  ///< Resonator or Self-Oscillator

    // Excitation parameters
    float poke_strength;        ///< Base excitation strength (0.0-1.0)
    float poke_duration_ms;     ///< Excitation envelope duration (1.0-50.0 ms)

    // Network behavior
    float coupling_response_gain;  ///< How strongly this node responds to coupling (0.5-1.5)

    // Metadata
    const char* name;           ///< Display name
    const char* description;    ///< Short description
};

// ============================================================================
// Character Library - 15 Built-in Characters
// ============================================================================

/**
 * @brief Character 0: Vibrant Bass
 *
 * Strong fundamental with harmonic overtones.
 * Low damping for sustained bass response.
 * Good for bass/root note roles in network.
 */
extern const NodeCharacter CHARACTER_VIBRANT_BASS;

/**
 * @brief Character 1: Dark Node
 *
 * Subdued upper modes, absorptive network behavior.
 * Higher damping for darker timbre.
 * Acts as energy sink in coupling network.
 */
extern const NodeCharacter CHARACTER_DARK_NODE;

/**
 * @brief Character 2: Bright Bell
 *
 * Inharmonic mode ratios (bell-like).
 * Strong upper mode presence.
 * Sharp attack, ringing sustain.
 */
extern const NodeCharacter CHARACTER_BRIGHT_BELL;

/**
 * @brief Character 3: Glassy Shimmer
 *
 * Near-harmonic with very high partial.
 * Shimmering, unstable quality.
 * Medium damping with emphasis on high mode.
 */
extern const NodeCharacter CHARACTER_GLASSY_SHIMMER;

/**
 * @brief Character 4: Drone Hub
 *
 * Near-unison low modes with beating.
 * Very low damping, self-oscillating.
 * Strong coupling response - network "hub" role.
 */
extern const NodeCharacter CHARACTER_DRONE_HUB;

/**
 * @brief Character 5: Metallic Strike
 *
 * Bright inharmonic ratios, fast decay.
 * Sharp metallic attack character.
 * High damping for percussive sound.
 */
extern const NodeCharacter CHARACTER_METALLIC_STRIKE;

/**
 * @brief Character 6: Warm Pad
 *
 * Perfect harmonics, ultra-low damping.
 * Smooth sustained pad sound.
 * Gentle long attack.
 */
extern const NodeCharacter CHARACTER_WARM_PAD;

/**
 * @brief Character 7: Percussive Hit
 *
 * Very high damping, fast decay.
 * Punchy percussive attack.
 * Minimal sustain.
 */
extern const NodeCharacter CHARACTER_PERCUSSIVE_HIT;

/**
 * @brief Character 8: Resonant Bell
 *
 * Harmonic stack, balanced sustain.
 * Bell-like resonance.
 * Medium attack and decay.
 */
extern const NodeCharacter CHARACTER_RESONANT_BELL;

/**
 * @brief Character 9: Deep Rumble
 *
 * Sub-bass focus, low partials.
 * Deep fundamental emphasis.
 * Sustained low end.
 */
extern const NodeCharacter CHARACTER_DEEP_RUMBLE;

/**
 * @brief Character 10: Harmonic Stack
 *
 * Perfect harmonic series (1,2,3,4).
 * Uniform damping.
 * Classic additive synthesis sound.
 */
extern const NodeCharacter CHARACTER_HARMONIC_STACK;

/**
 * @brief Character 11: Detuned Chorus
 *
 * Slightly detuned ratios.
 * Thick chorused sound.
 * Natural beating and movement.
 */
extern const NodeCharacter CHARACTER_DETUNED_CHORUS;

/**
 * @brief Character 12: Mallet Tone
 *
 * Wood mallet-like inharmonics.
 * Percussive with wood character.
 * Medium-fast decay.
 */
extern const NodeCharacter CHARACTER_MALLET_TONE;

/**
 * @brief Character 13: Wind Chime
 *
 * High delicate partials.
 * Shimmering bell-like quality.
 * Light and airy.
 */
extern const NodeCharacter CHARACTER_WIND_CHIME;

/**
 * @brief Character 14: Gong Wash
 *
 * Complex inharmonic ratios.
 * Evolving gong-like timbre.
 * Long sustained wash.
 */
extern const NodeCharacter CHARACTER_GONG_WASH;

/**
 * @brief Character library array for indexed access
 *
 * Maps character ID (0-14) to character definition.
 */
extern const NodeCharacter* CHARACTER_LIBRARY[15];

/**
 * @brief Number of built-in characters
 */
#define NUM_BUILTIN_CHARACTERS 15

// ============================================================================
// Character Utilities
// ============================================================================

/**
 * @brief Get character by ID
 * @param character_id Character index (0-4)
 * @return Pointer to character, or nullptr if invalid ID
 */
const NodeCharacter* getCharacter(uint8_t character_id);

/**
 * @brief Get character name by ID
 * @param character_id Character index (0-4)
 * @return Character name string, or "Unknown" if invalid
 */
const char* getCharacterName(uint8_t character_id);

/**
 * @brief Validate character parameters
 * @param character Character to validate
 * @return True if all parameters are in safe ranges
 */
bool validateCharacter(const NodeCharacter* character);

#endif // NODE_CHARACTER_H
