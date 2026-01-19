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
        5.0f,   // Fifth harmonic
        6.0f,   // Sixth harmonic
        7.0f    // Seventh harmonic
    },

    // Mode damping - low damping for sustained bass
    .mode_damping = {
        0.3f,   // Long fundamental decay
        0.5f,   // Medium second harmonic
        0.8f,   // Shorter third
        1.2f,   // Shortest fifth
        1.4f,   // Short sixth
        1.6f    // Short seventh
    },

    // Mode weights - fundamental-focused
    .mode_weight = {
        1.0f,   // Strong fundamental
        0.8f,   // Present second
        0.6f,   // Moderate third
        0.4f,   // Subtle fifth
        0.3f,   // Subtle sixth
        0.2f    // Subtle seventh
    },

    // Mode wave shapes - default to sine
    .mode_shape = {
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE
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
        3.1f,   // Dampened third
        4.3f,   // Compressed fourth
        5.8f    // Heavily dampened fifth
    },

    // Mode damping - strong damping for dark timbre
    .mode_damping = {
        0.8f,   // Moderate fundamental decay
        1.2f,   // Strong damping on fifth
        1.8f,   // Heavy damping on upper
        2.5f,   // Very heavy on highest
        3.0f,   // Very heavy on mode 4
        3.5f    // Extremely heavy on mode 5
    },

    // Mode weights - low brightness
    .mode_weight = {
        0.8f,   // Moderate fundamental
        0.4f,   // Subdued mid
        0.2f,   // Very quiet upper
        0.1f,   // Almost silent high
        0.05f,  // Nearly inaudible mode 4
        0.02f   // Nearly inaudible mode 5
    },

    // Mode wave shapes - default to sine
    .mode_shape = {
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE
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
        8.93f,   // Octave mode
        11.34f,  // Upper bell mode
        14.10f   // High shimmer mode
    },

    // Mode damping - mixed decay rates (bell characteristic)
    .mode_damping = {
        0.4f,   // Medium fundamental
        0.6f,   // Moderate second
        0.5f,   // Lighter third (rings)
        0.7f,   // Medium fourth
        0.8f,   // Medium-fast mode 4
        1.0f    // Faster mode 5
    },

    // Mode weights - bright upper emphasis
    .mode_weight = {
        0.7f,   // Present fundamental
        0.9f,   // Strong second
        1.0f,   // Full third (brightest)
        0.8f,   // Strong fourth
        0.6f,   // Moderate mode 4
        0.4f    // Subtle mode 5
    },

    // Mode wave shapes - default to sine
    .mode_shape = {
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE
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
        11.2f,   // Very high partial (shimmer)
        13.5f,   // Ultra-high shimmer
        16.8f    // Extreme high shimmer
    },

    // Mode damping - medium with shimmering high
    .mode_damping = {
        0.5f,   // Medium fundamental
        0.6f,   // Medium octave
        0.7f,   // Medium double octave
        0.4f,   // Lighter high (allows shimmer)
        0.5f,   // Medium-light mode 4
        0.6f    // Medium mode 5
    },

    // Mode weights - emphasize high mode
    .mode_weight = {
        0.6f,   // Moderate fundamental
        0.7f,   // Good octave presence
        0.6f,   // Moderate double octave
        0.9f,   // Strong high (shimmer effect)
        0.7f,   // Good ultra-high
        0.5f    // Moderate extreme high
    },

    // Mode wave shapes - default to sine
    .mode_shape = {
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE
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
        2.0f,     // Perfect octave
        2.995f,   // Just below perfect twelfth
        4.01f     // Slightly sharp double octave
    },

    // Mode damping - very low for sustained drone
    .mode_damping = {
        0.1f,    // Very long fundamental
        0.15f,   // Very long detuned
        0.2f,    // Long fifth
        0.3f,    // Long octave
        0.35f,   // Long twelfth
        0.4f     // Medium-long double octave
    },

    // Mode weights - rich low spectrum
    .mode_weight = {
        1.0f,    // Full fundamental
        0.9f,    // Nearly full detune (thick)
        0.7f,    // Good fifth presence
        0.5f,    // Moderate octave
        0.4f,    // Moderate twelfth
        0.3f     // Subtle double octave
    },

    // Mode wave shapes - default to sine
    .mode_shape = {
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE,
        WAVE_SHAPE_SINE
    },

    .personality = PERSONALITY_SELF_OSCILLATOR,  // Sustains continuously
    .poke_strength = 0.3f,      // Gentle - it sustains anyway
    .poke_duration_ms = 20.0f,  // Slow onset
    .coupling_response_gain = 1.2f,  // Strong coupling (hub role)

    .name = "Drone Hub",
    .description = "Self-sustaining drone with beating chorus effect"
};

const NodeCharacter CHARACTER_METALLIC_STRIKE = {
    .mode_freq_mult = { 1.0f, 3.14f, 5.87f, 8.23f, 10.92f, 13.45f },
    .mode_damping = { 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f },
    .mode_weight = { 0.6f, 0.8f, 1.0f, 0.7f, 0.5f, 0.3f },
    .mode_shape = { WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE },
    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.9f,
    .poke_duration_ms = 5.0f,
    .coupling_response_gain = 1.0f,
    .name = "Metallic Strike",
    .description = "Bright inharmonic strike with fast decay"
};

const NodeCharacter CHARACTER_WARM_PAD = {
    .mode_freq_mult = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f },
    .mode_damping = { 0.2f, 0.25f, 0.3f, 0.4f, 0.5f, 0.6f },
    .mode_weight = { 1.0f, 0.85f, 0.7f, 0.5f, 0.4f, 0.3f },
    .mode_shape = { WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE },
    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.3f,
    .poke_duration_ms = 30.0f,
    .coupling_response_gain = 0.7f,
    .name = "Warm Pad",
    .description = "Smooth sustained pad with perfect harmonics"
};

const NodeCharacter CHARACTER_PERCUSSIVE_HIT = {
    .mode_freq_mult = { 1.0f, 2.5f, 4.2f, 6.7f, 9.3f, 12.1f },
    .mode_damping = { 3.0f, 3.5f, 4.0f, 4.5f, 5.0f, 5.5f },
    .mode_weight = { 1.0f, 0.6f, 0.4f, 0.2f, 0.15f, 0.1f },
    .mode_shape = { WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE },
    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 1.0f,
    .poke_duration_ms = 3.0f,
    .coupling_response_gain = 0.8f,
    .name = "Percussive Hit",
    .description = "Fast decay percussive strike"
};

const NodeCharacter CHARACTER_RESONANT_BELL = {
    .mode_freq_mult = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f },
    .mode_damping = { 0.6f, 0.7f, 0.8f, 1.0f, 1.2f, 1.4f },
    .mode_weight = { 1.0f, 0.9f, 0.8f, 0.7f, 0.5f, 0.4f },
    .mode_shape = { WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE },
    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.75f,
    .poke_duration_ms = 12.0f,
    .coupling_response_gain = 1.0f,
    .name = "Resonant Bell",
    .description = "Harmonic bell with balanced sustain"
};

const NodeCharacter CHARACTER_DEEP_RUMBLE = {
    .mode_freq_mult = { 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f },
    .mode_damping = { 0.5f, 0.6f, 0.8f, 1.0f, 1.2f, 1.4f },
    .mode_weight = { 1.0f, 0.9f, 0.6f, 0.4f, 0.3f, 0.2f },
    .mode_shape = { WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE },
    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.6f,
    .poke_duration_ms = 20.0f,
    .coupling_response_gain = 0.9f,
    .name = "Deep Rumble",
    .description = "Sub-bass focus with low partials"
};

const NodeCharacter CHARACTER_HARMONIC_STACK = {
    .mode_freq_mult = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f },
    .mode_damping = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
    .mode_weight = { 1.0f, 0.8f, 0.6f, 0.4f, 0.3f, 0.2f },
    .mode_shape = { WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE },
    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.65f,
    .poke_duration_ms = 15.0f,
    .coupling_response_gain = 1.0f,
    .name = "Harmonic Stack",
    .description = "Perfect harmonic series with uniform damping"
};

const NodeCharacter CHARACTER_DETUNED_CHORUS = {
    .mode_freq_mult = { 1.0f, 1.99f, 2.98f, 4.03f, 4.97f, 6.02f },
    .mode_damping = { 0.7f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f },
    .mode_weight = { 1.0f, 0.85f, 0.7f, 0.5f, 0.4f, 0.3f },
    .mode_shape = { WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE },
    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.5f,
    .poke_duration_ms = 18.0f,
    .coupling_response_gain = 0.85f,
    .name = "Detuned Chorus",
    .description = "Slightly detuned for thick chorused sound"
};

const NodeCharacter CHARACTER_MALLET_TONE = {
    .mode_freq_mult = { 1.0f, 2.76f, 4.18f, 5.94f, 7.89f, 10.23f },
    .mode_damping = { 1.5f, 1.8f, 2.2f, 2.5f, 2.8f, 3.2f },
    .mode_weight = { 1.0f, 0.7f, 0.5f, 0.3f, 0.2f, 0.15f },
    .mode_shape = { WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE },
    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.85f,
    .poke_duration_ms = 8.0f,
    .coupling_response_gain = 0.9f,
    .name = "Mallet Tone",
    .description = "Wood mallet-like inharmonic character"
};

const NodeCharacter CHARACTER_WIND_CHIME = {
    .mode_freq_mult = { 3.0f, 4.5f, 6.2f, 8.7f, 11.5f, 14.8f },
    .mode_damping = { 0.9f, 1.0f, 1.1f, 1.3f, 1.5f, 1.7f },
    .mode_weight = { 0.7f, 0.8f, 1.0f, 0.8f, 0.6f, 0.4f },
    .mode_shape = { WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE },
    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.4f,
    .poke_duration_ms = 14.0f,
    .coupling_response_gain = 0.7f,
    .name = "Wind Chime",
    .description = "High delicate partials, light and airy"
};

const NodeCharacter CHARACTER_GONG_WASH = {
    .mode_freq_mult = { 1.0f, 2.37f, 3.86f, 5.19f, 6.84f, 8.57f },
    .mode_damping = { 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f },
    .mode_weight = { 0.8f, 1.0f, 0.9f, 0.7f, 0.6f, 0.5f },
    .mode_shape = { WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE },
    .personality = PERSONALITY_RESONATOR,
    .poke_strength = 0.7f,
    .poke_duration_ms = 35.0f,
    .coupling_response_gain = 1.1f,
    .name = "Gong Wash",
    .description = "Complex inharmonic wash with long sustain"
};

// ============================================================================
// Character Library Array
// ============================================================================

const NodeCharacter* CHARACTER_LIBRARY[NUM_BUILTIN_CHARACTERS] = {
    &CHARACTER_VIBRANT_BASS,
    &CHARACTER_DARK_NODE,
    &CHARACTER_BRIGHT_BELL,
    &CHARACTER_GLASSY_SHIMMER,
    &CHARACTER_DRONE_HUB,
    &CHARACTER_METALLIC_STRIKE,
    &CHARACTER_WARM_PAD,
    &CHARACTER_PERCUSSIVE_HIT,
    &CHARACTER_RESONANT_BELL,
    &CHARACTER_DEEP_RUMBLE,
    &CHARACTER_HARMONIC_STACK,
    &CHARACTER_DETUNED_CHORUS,
    &CHARACTER_MALLET_TONE,
    &CHARACTER_WIND_CHIME,
    &CHARACTER_GONG_WASH
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
    for (int i = 0; i < 6; i++) {
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
