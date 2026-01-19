/**
 * @file NodeCharacter.cpp
 * @brief Implementation of built-in node character library
 */

#include "NodeCharacter.h"
#include <cstring>

// ============================================================================
// Unified Character Library Array
// ============================================================================

const NodeCharacter CHARACTER_LIBRARY[] = {
    // ========================================================================
    // Characters 0-14: Original Character Designs
    // ========================================================================

    // 0: Vibrant Bass
    {
        .mode_freq_mult = {1.0f, 2.0f, 3.0f, 5.0f},
        .mode_damping = {0.3f, 0.5f, 0.8f, 1.2f},
        .mode_weight = {1.0f, 0.8f, 0.6f, 0.4f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.7f,
        .poke_duration_ms = 15.0f,
        .coupling_response_gain = 0.8f,
        .name = "Vibrant Bass",
        .description = "Strong harmonic bass with sustained low end"
    },

    // 1: Dark Node
    {
        .mode_freq_mult = {1.0f, 1.5f, 2.2f, 3.1f},
        .mode_damping = {0.8f, 1.2f, 1.8f, 2.5f},
        .mode_weight = {0.8f, 0.4f, 0.2f, 0.1f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.4f,
        .poke_duration_ms = 8.0f,
        .coupling_response_gain = 0.5f,
        .name = "Dark Node",
        .description = "Muted, absorptive character with low brightness"
    },

    // 2: Bright Bell
    {
        .mode_freq_mult = {1.0f, 2.76f, 5.40f, 8.93f},
        .mode_damping = {0.4f, 0.6f, 0.5f, 0.7f},
        .mode_weight = {0.7f, 0.9f, 1.0f, 0.8f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.6f,
        .poke_duration_ms = 5.0f,
        .coupling_response_gain = 1.0f,
        .name = "Bright Bell",
        .description = "Inharmonic bell-like tones with ringing highs"
    },

    // 3: Glassy Shimmer
    {
        .mode_freq_mult = {1.0f, 2.01f, 4.03f, 11.2f},
        .mode_damping = {0.5f, 0.6f, 0.7f, 0.4f},
        .mode_weight = {0.6f, 0.7f, 0.6f, 0.9f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.5f,
        .poke_duration_ms = 12.0f,
        .coupling_response_gain = 0.9f,
        .name = "Glassy Shimmer",
        .description = "Airy, shimmering high partials with instability"
    },

    // 4: Drone Hub
    {
        .mode_freq_mult = {1.0f, 1.002f, 1.498f, 2.0f},
        .mode_damping = {0.1f, 0.15f, 0.2f, 0.3f},
        .mode_weight = {1.0f, 0.9f, 0.7f, 0.5f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_SELF_OSCILLATOR,
        .poke_strength = 0.3f,
        .poke_duration_ms = 20.0f,
        .coupling_response_gain = 1.2f,
        .name = "Drone Hub",
        .description = "Self-sustaining drone with beating chorus effect"
    },

    // 5: Metallic Strike
    {
        .mode_freq_mult = {1.0f, 3.14f, 5.87f, 8.23f},
        .mode_damping = {2.0f, 2.5f, 3.0f, 3.5f},
        .mode_weight = {0.6f, 0.8f, 1.0f, 0.7f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.9f,
        .poke_duration_ms = 5.0f,
        .coupling_response_gain = 1.0f,
        .name = "Metallic Strike",
        .description = "Bright inharmonic strike with fast decay"
    },

    // 6: Warm Pad
    {
        .mode_freq_mult = {1.0f, 2.0f, 3.0f, 4.0f},
        .mode_damping = {0.2f, 0.25f, 0.3f, 0.4f},
        .mode_weight = {1.0f, 0.85f, 0.7f, 0.5f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.3f,
        .poke_duration_ms = 30.0f,
        .coupling_response_gain = 0.7f,
        .name = "Warm Pad",
        .description = "Smooth sustained pad with perfect harmonics"
    },

    // 7: Percussive Hit
    {
        .mode_freq_mult = {1.0f, 2.5f, 4.2f, 6.7f},
        .mode_damping = {3.0f, 3.5f, 4.0f, 4.5f},
        .mode_weight = {1.0f, 0.6f, 0.4f, 0.2f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 1.0f,
        .poke_duration_ms = 3.0f,
        .coupling_response_gain = 0.8f,
        .name = "Percussive Hit",
        .description = "Fast decay percussive strike"
    },

    // 8: Resonant Bell
    {
        .mode_freq_mult = {1.0f, 2.0f, 3.0f, 4.0f},
        .mode_damping = {0.6f, 0.7f, 0.8f, 1.0f},
        .mode_weight = {1.0f, 0.9f, 0.8f, 0.7f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.75f,
        .poke_duration_ms = 12.0f,
        .coupling_response_gain = 1.0f,
        .name = "Resonant Bell",
        .description = "Harmonic bell with balanced sustain"
    },

    // 9: Deep Rumble
    {
        .mode_freq_mult = {0.5f, 1.0f, 1.5f, 2.0f},
        .mode_damping = {0.5f, 0.6f, 0.8f, 1.0f},
        .mode_weight = {1.0f, 0.9f, 0.6f, 0.4f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.6f,
        .poke_duration_ms = 20.0f,
        .coupling_response_gain = 0.9f,
        .name = "Deep Rumble",
        .description = "Sub-bass focus with low partials"
    },

    // 10: Harmonic Stack
    {
        .mode_freq_mult = {1.0f, 2.0f, 3.0f, 4.0f},
        .mode_damping = {1.0f, 1.0f, 1.0f, 1.0f},
        .mode_weight = {1.0f, 0.8f, 0.6f, 0.4f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.65f,
        .poke_duration_ms = 15.0f,
        .coupling_response_gain = 1.0f,
        .name = "Harmonic Stack",
        .description = "Perfect harmonic series with uniform damping"
    },

    // 11: Detuned Chorus
    {
        .mode_freq_mult = {1.0f, 1.99f, 2.98f, 4.03f},
        .mode_damping = {0.7f, 0.7f, 0.8f, 0.9f},
        .mode_weight = {1.0f, 0.85f, 0.7f, 0.5f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.5f,
        .poke_duration_ms = 18.0f,
        .coupling_response_gain = 0.85f,
        .name = "Detuned Chorus",
        .description = "Slightly detuned for thick chorused sound"
    },

    // 12: Mallet Tone
    {
        .mode_freq_mult = {1.0f, 2.76f, 4.18f, 5.94f},
        .mode_damping = {1.5f, 1.8f, 2.2f, 2.5f},
        .mode_weight = {1.0f, 0.7f, 0.5f, 0.3f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.85f,
        .poke_duration_ms = 8.0f,
        .coupling_response_gain = 0.9f,
        .name = "Mallet Tone",
        .description = "Wood mallet-like inharmonic character"
    },

    // 13: Wind Chime
    {
        .mode_freq_mult = {3.0f, 4.5f, 6.2f, 8.7f},
        .mode_damping = {0.9f, 1.0f, 1.1f, 1.3f},
        .mode_weight = {0.7f, 0.8f, 1.0f, 0.8f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.4f,
        .poke_duration_ms = 14.0f,
        .coupling_response_gain = 0.7f,
        .name = "Wind Chime",
        .description = "High delicate partials, light and airy"
    },

    // 14: Gong Wash
    {
        .mode_freq_mult = {1.0f, 2.37f, 3.86f, 5.19f},
        .mode_damping = {0.4f, 0.5f, 0.6f, 0.7f},
        .mode_weight = {0.8f, 1.0f, 0.9f, 0.7f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.7f,
        .poke_duration_ms = 35.0f,
        .coupling_response_gain = 1.1f,
        .name = "Gong Wash",
        .description = "Complex inharmonic wash with long sustain"
    },

    // ========================================================================
    // Characters 15-20: Physically-Derived Presets
    // ========================================================================

    // 15: Church Bell
    {
        .mode_freq_mult = {1.0f, 1.19f, 1.5f, 2.0f},
        .mode_damping = {1.0f, 1.2f, 1.4f, 1.8f},
        .mode_weight = {1.0f, 0.75f, 0.6f, 0.45f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.7f,
        .poke_duration_ms = 10.0f,
        .coupling_response_gain = 1.0f,
        .name = "Church Bell",
        .description = "Western church bell with hum, fundamental, tierce, and quint"
    },

    // 16: Circular Plate
    {
        .mode_freq_mult = {1.0f, 2.081f, 3.413f, 3.891f},
        .mode_damping = {1.0f, 1.1f, 1.3f, 1.4f},
        .mode_weight = {1.0f, 0.7f, 0.5f, 0.35f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.8f,
        .poke_duration_ms = 8.0f,
        .coupling_response_gain = 1.0f,
        .name = "Circular Plate",
        .description = "Flat circular plate (cymbal, gong) - Rayleigh modes"
    },

    // 17: Wine Glass
    {
        .mode_freq_mult = {1.0f, 2.28f, 3.65f, 5.13f},
        .mode_damping = {1.0f, 1.3f, 1.6f, 2.0f},
        .mode_weight = {1.0f, 0.65f, 0.45f, 0.3f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.6f,
        .poke_duration_ms = 12.0f,
        .coupling_response_gain = 0.9f,
        .name = "Wine Glass",
        .description = "Cylindrical shell resonance (wine glass rim)"
    },

    // 18: Tuned Bar
    {
        .mode_freq_mult = {1.0f, 4.0f, 10.0f, 18.0f},
        .mode_damping = {1.0f, 1.4f, 2.0f, 2.5f},
        .mode_weight = {1.0f, 0.5f, 0.25f, 0.15f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.75f,
        .poke_duration_ms = 10.0f,
        .coupling_response_gain = 0.95f,
        .name = "Tuned Bar",
        .description = "Arch-tuned bar (marimba, xylophone) with harmonic overtones"
    },

    // 19: Drum Membrane
    {
        .mode_freq_mult = {1.0f, 1.593f, 2.136f, 2.296f},
        .mode_damping = {0.8f, 1.0f, 1.2f, 1.3f},
        .mode_weight = {1.0f, 0.7f, 0.5f, 0.4f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.8f,
        .poke_duration_ms = 15.0f,
        .coupling_response_gain = 0.85f,
        .name = "Drum Membrane",
        .description = "Circular membrane (kettledrum, timpani)"
    },

    // 20: Small Bell
    {
        .mode_freq_mult = {1.0f, 1.35f, 1.7f, 2.2f},
        .mode_damping = {0.6f, 0.8f, 1.0f, 1.2f},
        .mode_weight = {1.0f, 0.8f, 0.6f, 0.4f},
        .mode_shape = {WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE, WAVE_SHAPE_SINE},
        .personality = PERSONALITY_RESONATOR,
        .poke_strength = 0.7f,
        .poke_duration_ms = 7.0f,
        .coupling_response_gain = 1.0f,
        .name = "Small Bell",
        .description = "Small handbell or bicycle bell (higher inharmonicity)"
    }
};

// ============================================================================
// Legacy Utility Functions (deprecated - use CharacterManager instead)
// ============================================================================

const NodeCharacter* getCharacter(uint8_t character_id) {
    if (character_id >= NUM_BUILTIN_CHARACTERS) {
        return nullptr;
    }
    return &CHARACTER_LIBRARY[character_id];
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
