/**
 * @file NodeManager.h
 * @brief Fixed 5-node network management system
 *
 * Replaces VoiceAllocator with a character-based node network.
 * Key differences:
 * - Fixed 5 nodes (always exist, never allocated/freed)
 * - Each node has a character (sonic identity)
 * - Notes excite nodes based on routing strategy
 * - No voice stealing (nodes can be re-excited while ringing)
 */

#ifndef NODE_MANAGER_H
#define NODE_MANAGER_H

#include "ModalVoice.h"
#include "NodeCharacter.h"
#include <cstdint>

/**
 * @brief Number of nodes in the network (fixed)
 */
#define NUM_NETWORK_NODES 5

/**
 * @brief Note routing strategies
 */
enum class NoteRoutingMode : uint8_t {
    MidiChannel = 0,   ///< Route by MIDI channel (Ch 1→Node 0, Ch 2→Node 1, etc.)
    AllNodes = 1,      ///< All nodes receive every note
};

/**
 * @brief Multi-excitation modes
 */
enum class MultiExciteMode : uint8_t {
    ReTrigger = 0,     ///< New note replaces old (re-trigger node)
    Accumulate = 1,    ///< New note adds to existing excitation (accumulate)
};

/**
 * @brief Fixed 5-node network manager
 *
 * Manages a persistent network of 5 nodes, each with its own character.
 * Handles note routing, character application, and network rendering.
 */
class NodeManager {
public:
    /**
     * @brief Constructor
     *
     * Always creates exactly 5 nodes.
     */
    NodeManager();

    /**
     * @brief Destructor
     */
    ~NodeManager();

    /**
     * @brief Initialize manager
     * @param sample_rate Sample rate in Hz
     */
    void initialize(float sample_rate);

    // ========================================================================
    // Character Management
    // ========================================================================

    /**
     * @brief Set character for a specific node
     * @param node_idx Node index (0-4)
     * @param character_id Character ID (0-4)
     */
    void setNodeCharacter(uint8_t node_idx, uint8_t character_id);

    /**
     * @brief Apply custom character to node
     * @param node_idx Node index (0-4)
     * @param character Pointer to character definition
     */
    void setNodeCharacterCustom(uint8_t node_idx, const NodeCharacter* character);

    /**
     * @brief Get current character ID for node
     * @param node_idx Node index (0-4)
     * @return Character ID, or 0xFF if invalid node
     */
    uint8_t getNodeCharacterID(uint8_t node_idx) const;

    /**
     * @brief Set wave shape for a specific mode
     * @param node_idx Node index (0-4)
     * @param mode_idx Mode index (0-3)
     * @param shape Wave shape to use
     */
    void setModeWaveShape(uint32_t node_idx, uint32_t mode_idx, wave_shape_t shape);

    /**
     * @brief Get wave shape for a specific mode
     * @param node_idx Node index (0-4)
     * @param mode_idx Mode index (0-3)
     * @return Current wave shape
     */
    wave_shape_t getModeWaveShape(uint32_t node_idx, uint32_t mode_idx) const;

    // ========================================================================
    // Routing Configuration
    // ========================================================================

    /**
     * @brief Set note routing mode
     * @param mode Routing strategy
     */
    void setRoutingMode(NoteRoutingMode mode) {
        routing_mode_ = mode;
    }

    /**
     * @brief Get current routing mode
     * @return Current routing strategy
     */
    NoteRoutingMode getRoutingMode() const {
        return routing_mode_;
    }

    /**
     * @brief Set multi-excitation mode
     * @param mode Excitation behavior
     */
    void setMultiExciteMode(MultiExciteMode mode) {
        multi_excite_mode_ = mode;
    }

    /**
     * @brief Get current multi-excitation mode
     * @return Current excitation behavior
     */
    MultiExciteMode getMultiExciteMode() const {
        return multi_excite_mode_;
    }

    /**
     * @brief Set active node count (1-5)
     * @param count Number of active nodes
     *
     * Calling this will trigger allNotesOff() for safety.
     */
    void setNodeCount(uint8_t count);

    /**
     * @brief Get active node count
     * @return Number of active nodes (1-5)
     */
    uint8_t getNodeCount() const {
        return active_node_count_;
    }

    /**
     * @brief Set global damping for all nodes
     * @param damping Global damping coefficient (>= 0.0)
     *
     * This adds extra damping to all modal oscillators, effectively
     * removing energy from the system. Can be used to calm runaway
     * oscillations or control overall system energy.
     */
    void setGlobalDamping(float damping);

    // ========================================================================
    // Note Handling
    // ========================================================================

    /**
     * @brief Handle MIDI note on
     * @param midi_note MIDI note number (0-127)
     * @param velocity Velocity (0.0-1.0 normalized)
     * @param midi_channel MIDI channel (0-15, where 0 = channel 1)
     *
     * Routes note to node(s) based on routing mode.
     * Applies excitation according to multi-excite mode.
     */
    void noteOn(uint8_t midi_note, float velocity, uint8_t midi_channel = 0);

    /**
     * @brief Handle MIDI note off
     * @param midi_note MIDI note number (0-127)
     *
     * Releases node(s) associated with this note.
     */
    void noteOff(uint8_t midi_note);

    /**
     * @brief Release all nodes
     */
    void allNotesOff();

    /**
     * @brief Apply pitch bend to all active nodes
     * @param bend_amount Pitch bend amount (-1.0 to +1.0)
     */
    void setPitchBend(float bend_amount);

    // ========================================================================
    // Direct Node Access
    // ========================================================================

    /**
     * @brief Get direct access to node
     * @param node_idx Node index (0-4)
     * @return Pointer to node, or nullptr if invalid
     */
    ModalVoice* getNode(uint8_t node_idx);

    /**
     * @brief Excite specific node directly
     * @param node_idx Node index (0-4)
     * @param midi_note MIDI note to excite with
     * @param velocity Excitation strength (0.0-1.0)
     */
    void exciteNode(uint8_t node_idx, uint8_t midi_note, float velocity);

    /**
     * @brief Release specific node
     * @param node_idx Node index (0-4)
     */
    void releaseNode(uint8_t node_idx);

    // ========================================================================
    // Rendering
    // ========================================================================

    /**
     * @brief Update all nodes at control rate
     *
     * Should be called at control rate (500 Hz typically)
     */
    void updateNodes();

    /**
     * @brief Render audio from all nodes
     * @param outL Left channel output buffer
     * @param outR Right channel output buffer
     * @param num_frames Number of frames to render
     */
    void renderAudio(float* outL, float* outR, uint32_t num_frames);

    // ========================================================================
    // Status
    // ========================================================================

    /**
     * @brief Get number of active nodes
     * @return Count of nodes currently sounding
     */
    uint32_t getActiveNodeCount() const;

    /**
     * @brief Check if node is active
     * @param node_idx Node index (0-4)
     * @return True if node is currently sounding
     */
    bool isNodeActive(uint8_t node_idx) const;

private:
    // Network nodes (fixed 5)
    ModalVoice* nodes_[NUM_NETWORK_NODES];  ///< Fixed array of 5 nodes

    // Character tracking
    uint8_t node_character_ids_[NUM_NETWORK_NODES];  ///< Current character per node
    NodeCharacter current_characters_[NUM_NETWORK_NODES];  ///< Active character data

    // Routing state
    NoteRoutingMode routing_mode_;          ///< Current routing strategy
    MultiExciteMode multi_excite_mode_;     ///< Current excitation behavior
    uint8_t active_node_count_;             ///< Number of active nodes (1-5)

    // Note tracking (for note-off routing)
    uint8_t note_to_node_[128];             ///< MIDI note → node mapping (-1 = none)
    uint8_t note_to_channel_[128];          ///< MIDI note → channel mapping

    // Global state
    float pitch_bend_;                      ///< Current pitch bend amount
    float sample_rate_;                     ///< Current sample rate
    bool initialized_;                      ///< Initialization flag

    // Pre-allocated temp buffers (real-time safe)
    float* temp_buffer_L_;                  ///< Temp buffer for rendering (L)
    float* temp_buffer_R_;                  ///< Temp buffer for rendering (R)
    uint32_t max_buffer_size_;              ///< Max buffer size allocated

    /**
     * @brief Route note to node index(es) based on current routing mode
     * @param midi_note MIDI note number
     * @param midi_channel MIDI channel (0-15)
     * @param target_nodes Output array of node indices to excite
     * @return Number of nodes to excite
     */
    uint8_t routeNoteToNodes(uint8_t midi_note, uint8_t midi_channel, uint8_t* target_nodes);

    /**
     * @brief Apply character parameters to node
     * @param node_idx Node index
     * @param character Character definition
     */
    void applyCharacterToNode(uint8_t node_idx, const NodeCharacter* character);
};

#endif // NODE_MANAGER_H
