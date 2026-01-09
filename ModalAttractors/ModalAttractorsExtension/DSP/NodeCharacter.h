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
 * - Personality (resonator vs self-oscillator)
 * - Excitation behavior (poke strength/duration)
 * - Network coupling response
 */
struct NodeCharacter {
    // Mode parameters (4 modes per node)
    float mode_freq_mult[4];    ///< Frequency multipliers relative to base note
    float mode_damping[4];      ///< Damping coefficients (higher = faster decay)
    float mode_weight[4];       ///< Audio contribution weights (0.0-1.0)

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
// Character Library - 5 Built-in Characters
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
 * @brief Character library array for indexed access
 *
 * Maps character ID (0-4) to character definition.
 */
extern const NodeCharacter* CHARACTER_LIBRARY[5];

/**
 * @brief Number of built-in characters
 */
#define NUM_BUILTIN_CHARACTERS 5

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
