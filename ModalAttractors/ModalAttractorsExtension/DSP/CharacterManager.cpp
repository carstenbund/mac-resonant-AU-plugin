/**
 * @file CharacterManager.cpp
 * @brief Implementation of CharacterManager
 */

#include "CharacterManager.h"
#include <cstring>

const NodeCharacter& CharacterManager::getCharacter(size_t index) {
    if (index >= getCharacterCount()) {
        // Return default character (first one)
        return CHARACTER_LIBRARY[0];
    }
    return CHARACTER_LIBRARY[index];
}

size_t CharacterManager::getCharacterCount() {
    return NUM_BUILTIN_CHARACTERS;
}

std::optional<size_t> CharacterManager::findCharacterByName(const std::string& name) {
    for (size_t i = 0; i < getCharacterCount(); ++i) {
        if (name == CHARACTER_LIBRARY[i].name) {
            return i;
        }
    }
    return std::nullopt;
}

const NodeCharacter& CharacterManager::getCharacterByNameOrDefault(
    const std::string& name,
    size_t fallback_index
) {
    auto index = findCharacterByName(name);
    return getCharacter(index.value_or(fallback_index));
}

bool CharacterManager::validateCharacter(const NodeCharacter& character) {
    // Validate num_active_modes is within range
    if (character.num_active_modes < 1 || character.num_active_modes > MAX_MODES) {
        return false;
    }

    // Validate mode parameters (only check active modes)
    for (int i = 0; i < character.num_active_modes; i++) {
        // Frequency multipliers should be positive and reasonable
        if (character.mode_freq_mult[i] < 0.1f || character.mode_freq_mult[i] > 20.0f) {
            return false;
        }

        // Damping should be positive and reasonable
        if (character.mode_damping[i] < 0.01f || character.mode_damping[i] > 10.0f) {
            return false;
        }

        // Weights should be 0-1
        if (character.mode_weight[i] < 0.0f || character.mode_weight[i] > 1.0f) {
            return false;
        }
    }

    // Validate excitation parameters
    if (character.poke_strength < 0.0f || character.poke_strength > 1.0f) {
        return false;
    }

    if (character.poke_duration_ms < 1.0f || character.poke_duration_ms > 50.0f) {
        return false;
    }

    // Validate coupling response (allow some headroom)
    if (character.coupling_response_gain < 0.1f || character.coupling_response_gain > 2.0f) {
        return false;
    }

    return true;
}

const char* CharacterManager::getCharacterName(size_t index) {
    return getCharacter(index).name;
}

const char* CharacterManager::getCharacterDescription(size_t index) {
    return getCharacter(index).description;
}
