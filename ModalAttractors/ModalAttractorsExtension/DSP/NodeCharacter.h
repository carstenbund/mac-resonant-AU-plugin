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
 * - Modal frequency ratios (up to 8 modes, configurable per character)
 * - Modal damping coefficients (up to 8 modes)
 * - Modal audio weights (up to 8 modes)
 * - Modal wave shapes (up to 8 modes)
 * - Personality (resonator vs self-oscillator)
 * - Excitation behavior (poke strength/duration)
 * - Network coupling response
 */
struct NodeCharacter {
    // Number of active modes for this character (1-8)
    uint8_t num_active_modes;   ///< How many modes are used (remaining slots ignored)

    // Mode parameters (up to 8 modes per node)
    float mode_freq_mult[MAX_MODES];    ///< Frequency multipliers relative to base note
    float mode_damping[MAX_MODES];      ///< Damping coefficients (higher = faster decay)
    float mode_weight[MAX_MODES];       ///< Audio contribution weights (0.0-1.0)
    wave_shape_t mode_shape[MAX_MODES]; ///< Wave shapes for each mode

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
// Character Library - 21 Built-in Characters
// ============================================================================

/**
 * @brief Character library array
 *
 * Unified array containing all built-in characters.
 * Character IDs 0-14: Original characters
 * Character IDs 15-20: Physically-derived presets
 *
 * Access via CharacterManager for bounds checking and validation.
 */
extern const NodeCharacter CHARACTER_LIBRARY[];

/**
 * @brief Number of built-in characters
 */
#define NUM_BUILTIN_CHARACTERS 21

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
