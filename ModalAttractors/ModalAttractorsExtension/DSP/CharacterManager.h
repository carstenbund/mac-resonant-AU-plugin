/**
 * @file CharacterManager.h
 * @brief Manages character library access and validation
 *
 * Provides unified API for character retrieval, validation,
 * and optional user preset management.
 */

#ifndef CHARACTER_MANAGER_H
#define CHARACTER_MANAGER_H

#include "NodeCharacter.h"
#include <string>
#include <optional>
#include <cstdint>

/**
 * @brief Manages character library access and validation
 *
 * Provides unified API for character retrieval, validation,
 * and optional user preset management.
 */
class CharacterManager {
public:
    /**
     * @brief Get character by index
     * @param index Character ID (0 to getCharacterCount()-1)
     * @return Const reference to character, or default character if invalid
     */
    static const NodeCharacter& getCharacter(size_t index);

    /**
     * @brief Get total number of built-in characters
     * @return Character count
     */
    static size_t getCharacterCount();

    /**
     * @brief Find character index by name
     * @param name Character name (case-sensitive)
     * @return Character index if found, std::nullopt otherwise
     */
    static std::optional<size_t> findCharacterByName(const std::string& name);

    /**
     * @brief Get character by name with fallback
     * @param name Character name
     * @param fallback_index Fallback index if name not found (default: 0)
     * @return Const reference to character
     */
    static const NodeCharacter& getCharacterByNameOrDefault(
        const std::string& name,
        size_t fallback_index = 0
    );

    /**
     * @brief Validate character parameters
     * @param character Character to validate
     * @return True if valid, false otherwise
     */
    static bool validateCharacter(const NodeCharacter& character);

    /**
     * @brief Get character name by index
     * @param index Character ID
     * @return Character name, or "Unknown" if invalid
     */
    static const char* getCharacterName(size_t index);

    /**
     * @brief Get character description by index
     * @param index Character ID
     * @return Character description, or "" if invalid
     */
    static const char* getCharacterDescription(size_t index);

    // Future expansion: user preset management
    // static bool saveUserCharacter(const NodeCharacter& character, const std::string& name);
    // static std::vector<NodeCharacter> getUserCharacters();
};

#endif // CHARACTER_MANAGER_H
