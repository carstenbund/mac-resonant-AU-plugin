/**
 * @file NodeCharacter.cpp
 * @brief Implementation of built-in node character library
 */

#include "NodeCharacter.h"
#include <cstring>

// ============================================================================
// Character Definitions
// ============================================================================

const NodeCharacter CHARACTER_VIBRANT_BASS = {
    // Mode frequency multipliers - harmonic series
    .mode_freq_mult = {
        1.0f,   // Fundamental
        2.0f,   // Second harmonic
        3.0f,   // Third harmonic
        5.0f    // Fifth harmonic
    },

    // Mode damping - low damping for sustained bass
    .mode_damping = {
        0.3f,   // Long fundamental decay
        0.5f,   // Medium second harmonic
        0.8f,   // Shorter third
        1.2f    // Shortest fifth
    },

    // Mode weights - fundamental-focused
    .mode_weight = {
        1.0f,   // Strong fundamental
        0.8f,   // Present second
        0.6f,   // Moderate third
        0.4f    // Subtle fifth
    },

    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.7f,
    .poke_duration_ms = 15.0f,
    .coupling_response_gain = 0.8f,

    .name = "Vibrant Bass",
    .description = "Strong harmonic bass with sustained low end"
};

const NodeCharacter CHARACTER_DARK_NODE = {
    // Mode frequency multipliers - subdued upper modes
    .mode_freq_mult = {
        1.0f,   // Fundamental
        1.5f,   // Perfect fifth (muted)
        2.2f,   // Compressed second harmonic
        3.1f    // Dampened third
    },

    // Mode damping - strong damping for dark timbre
    .mode_damping = {
        0.8f,   // Moderate fundamental decay
        1.2f,   // Strong damping on fifth
        1.8f,   // Heavy damping on upper
        2.5f    // Very heavy on highest
    },

    // Mode weights - low brightness
    .mode_weight = {
        0.8f,   // Moderate fundamental
        0.4f,   // Subdued mid
        0.2f,   // Very quiet upper
        0.1f    // Almost silent high
    },

    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.4f,
    .poke_duration_ms = 8.0f,
    .coupling_response_gain = 0.5f,  // Absorptive in network

    .name = "Dark Node",
    .description = "Muted, absorptive character with low brightness"
};

const NodeCharacter CHARACTER_BRIGHT_BELL = {
    // Mode frequency multipliers - inharmonic (bell-like ratios)
    // Based on typical bell mode ratios
    .mode_freq_mult = {
        1.0f,    // Fundamental
        2.76f,   // Minor third mode
        5.40f,   // Fifth mode
        8.93f    // Octave mode
    },

    // Mode damping - mixed decay rates (bell characteristic)
    .mode_damping = {
        0.4f,   // Medium fundamental
        0.6f,   // Moderate second
        0.5f,   // Lighter third (rings)
        0.7f    // Medium fourth
    },

    // Mode weights - bright upper emphasis
    .mode_weight = {
        0.7f,   // Present fundamental
        0.9f,   // Strong second
        1.0f,   // Full third (brightest)
        0.8f    // Strong fourth
    },

    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.6f,
    .poke_duration_ms = 5.0f,  // Sharp attack
    .coupling_response_gain = 1.0f,

    .name = "Bright Bell",
    .description = "Inharmonic bell-like tones with ringing highs"
};

const NodeCharacter CHARACTER_GLASSY_SHIMMER = {
    // Mode frequency multipliers - near-harmonic with very high partial
    .mode_freq_mult = {
        1.0f,    // Fundamental
        2.01f,   // Slightly detuned octave (beating)
        4.03f,   // Slightly detuned double octave
        11.2f    // Very high partial (shimmer)
    },

    // Mode damping - medium with shimmering high
    .mode_damping = {
        0.5f,   // Medium fundamental
        0.6f,   // Medium octave
        0.7f,   // Medium double octave
        0.4f    // Lighter high (allows shimmer)
    },

    // Mode weights - emphasize high mode
    .mode_weight = {
        0.6f,   // Moderate fundamental
        0.7f,   // Good octave presence
        0.6f,   // Moderate double octave
        0.9f    // Strong high (shimmer effect)
    },

    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.5f,
    .poke_duration_ms = 12.0f,
    .coupling_response_gain = 0.9f,

    .name = "Glassy Shimmer",
    .description = "Airy, shimmering high partials with instability"
};

const NodeCharacter CHARACTER_DRONE_HUB = {
    // Mode frequency multipliers - near-unison for beating/chorus
    .mode_freq_mult = {
        1.0f,     // Fundamental
        1.002f,   // Very slight detune (slow beating)
        1.498f,   // Just below perfect fifth (tension)
        2.0f      // Perfect octave
    },

    // Mode damping - very low for sustained drone
    .mode_damping = {
        0.1f,    // Very long fundamental
        0.15f,   // Very long detuned
        0.2f,    // Long fifth
        0.3f     // Long octave
    },

    // Mode weights - rich low spectrum
    .mode_weight = {
        1.0f,    // Full fundamental
        0.9f,    // Nearly full detune (thick)
        0.7f,    // Good fifth presence
        0.5f     // Moderate octave
    },

    .personality = PERSONALITY_SELF_OSCILLATOR,  // Sustains continuously
    .poke_strength = 0.3f,      // Gentle - it sustains anyway
    .poke_duration_ms = 20.0f,  // Slow onset
    .coupling_response_gain = 1.2f,  // Strong coupling (hub role)

    .name = "Drone Hub",
    .description = "Self-sustaining drone with beating chorus effect"
};

// ============================================================================
// Character Library Array
// ============================================================================

const NodeCharacter* CHARACTER_LIBRARY[NUM_BUILTIN_CHARACTERS] = {
    &CHARACTER_VIBRANT_BASS,
    &CHARACTER_DARK_NODE,
    &CHARACTER_BRIGHT_BELL,
    &CHARACTER_GLASSY_SHIMMER,
    &CHARACTER_DRONE_HUB
};

// ============================================================================
// Utility Functions
// ============================================================================

const NodeCharacter* getCharacter(uint8_t character_id) {
    if (character_id >= NUM_BUILTIN_CHARACTERS) {
        return nullptr;
    }
    return CHARACTER_LIBRARY[character_id];
}

const char* getCharacterName(uint8_t character_id) {
    const NodeCharacter* character = getCharacter(character_id);
    if (character) {
        return character->name;
    }
    return "Unknown";
}

bool validateCharacter(const NodeCharacter* character) {
    if (!character) return false;

    // Validate mode parameters
    for (int i = 0; i < 4; i++) {
        // Frequency multipliers should be positive and reasonable
        if (character->mode_freq_mult[i] < 0.1f || character->mode_freq_mult[i] > 20.0f) {
            return false;
        }

        // Damping should be positive and reasonable
        if (character->mode_damping[i] < 0.01f || character->mode_damping[i] > 10.0f) {
            return false;
        }

        // Weights should be 0-1
        if (character->mode_weight[i] < 0.0f || character->mode_weight[i] > 1.0f) {
            return false;
        }
    }

    // Validate excitation parameters
    if (character->poke_strength < 0.0f || character->poke_strength > 1.0f) {
        return false;
    }

    if (character->poke_duration_ms < 1.0f || character->poke_duration_ms > 50.0f) {
        return false;
    }

    // Validate coupling response (allow some headroom)
    if (character->coupling_response_gain < 0.1f || character->coupling_response_gain > 2.0f) {
        return false;
    }

    return true;
}
