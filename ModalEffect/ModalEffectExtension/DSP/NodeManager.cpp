/**
 * @file NodeManager.cpp
 * @brief Implementation of 5-node network manager
 */

#include "NodeManager.h"
#include <cstring>
#include <algorithm>

NodeManager::NodeManager()
    : routing_mode_(NoteRoutingMode::MidiChannel)
    , multi_excite_mode_(MultiExciteMode::Accumulate)
    , active_node_count_(5)        // Default: all 5 nodes active
    , pitch_bend_(0.0f)
    , sample_rate_(48000.0f)
    , initialized_(false)
    , temp_buffer_L_(nullptr)
    , temp_buffer_R_(nullptr)
    , max_buffer_size_(0)
{
    // Create exactly 5 nodes
    for (uint8_t i = 0; i < NUM_NETWORK_NODES; i++) {
        nodes_[i] = new ModalVoice(i);
        node_character_ids_[i] = i;  // Default: each node gets its own character
    }

    // Initialize note mapping
    memset(note_to_node_, 0xFF, sizeof(note_to_node_));  // 0xFF = no node
    memset(note_to_channel_, 0xFF, sizeof(note_to_channel_));
}

NodeManager::~NodeManager() {
    // Delete all nodes
    for (uint8_t i = 0; i < NUM_NETWORK_NODES; i++) {
        if (nodes_[i]) {
            delete nodes_[i];
            nodes_[i] = nullptr;
        }
    }

    // Free temp buffers
    if (temp_buffer_L_) {
        delete[] temp_buffer_L_;
        temp_buffer_L_ = nullptr;
    }
    if (temp_buffer_R_) {
        delete[] temp_buffer_R_;
        temp_buffer_R_ = nullptr;
    }
}

void NodeManager::initialize(float sample_rate) {
    sample_rate_ = sample_rate;

    // Initialize all nodes
    for (uint8_t i = 0; i < NUM_NETWORK_NODES; i++) {
        nodes_[i]->initialize(sample_rate);

        // Apply default character
        setNodeCharacter(i, node_character_ids_[i]);
    }

    // Allocate temp buffers (real-time safe rendering)
    max_buffer_size_ = 2048;  // Covers most typical buffer sizes
    temp_buffer_L_ = new float[max_buffer_size_];
    temp_buffer_R_ = new float[max_buffer_size_];

    initialized_ = true;
}

// ============================================================================
// Character Management
// ============================================================================

void NodeManager::setNodeCharacter(uint8_t node_idx, uint8_t character_id) {
    if (node_idx >= NUM_NETWORK_NODES) return;
    if (character_id >= NUM_BUILTIN_CHARACTERS) return;

    const NodeCharacter* character = CHARACTER_LIBRARY[character_id];
    if (!character || !validateCharacter(character)) return;

    // Store character ID and data
    node_character_ids_[node_idx] = character_id;
    current_characters_[node_idx] = *character;

    // Apply to node
    applyCharacterToNode(node_idx, character);
}

void NodeManager::setNodeCharacterCustom(uint8_t node_idx, const NodeCharacter* character) {
    if (node_idx >= NUM_NETWORK_NODES) return;
    if (!character || !validateCharacter(character)) return;

    // Store custom character (ID = 0xFF for custom)
    node_character_ids_[node_idx] = 0xFF;
    current_characters_[node_idx] = *character;

    // Apply to node
    applyCharacterToNode(node_idx, character);
}

uint8_t NodeManager::getNodeCharacterID(uint8_t node_idx) const {
    if (node_idx >= NUM_NETWORK_NODES) return 0xFF;
    return node_character_ids_[node_idx];
}

void NodeManager::setModeWaveShape(uint32_t node_idx, uint32_t mode_idx, wave_shape_t shape) {
    if (node_idx >= NUM_NETWORK_NODES) return;
    if (mode_idx >= MAX_MODES) return;

    ModalVoice* node = nodes_[node_idx];
    if (!node) return;

    // Set wave shape for the specified mode
    modal_node_t* modal = node->getModalNode();
    if (modal) {
        modal->modes[mode_idx].params.shape = shape;
    }
}

wave_shape_t NodeManager::getModeWaveShape(uint32_t node_idx, uint32_t mode_idx) const {
    if (node_idx >= NUM_NETWORK_NODES) return WAVE_SHAPE_SINE;
    if (mode_idx >= MAX_MODES) return WAVE_SHAPE_SINE;

    ModalVoice* node = nodes_[node_idx];
    if (!node) return WAVE_SHAPE_SINE;

    // Get wave shape for the specified mode
    modal_node_t* modal = node->getModalNode();
    if (modal) {
        return modal->modes[mode_idx].params.shape;
    }
    return WAVE_SHAPE_SINE;
}

void NodeManager::applyCharacterToNode(uint8_t node_idx, const NodeCharacter* character) {
    if (!initialized_) return;

    ModalVoice* node = nodes_[node_idx];
    if (!node) return;

    // Apply personality
    node->setPersonality(character->personality);

    // Apply mode parameters
    // Note: We'll apply frequency multipliers on note-on when we know the base frequency
    // For now, just store them in the character struct
    // The frequencies will be applied in exciteNode()

    // Apply wave shapes from character to each mode
    modal_node_t* modal = node->getModalNode();
    if (modal) {
        for (int i = 0; i < MAX_MODES; i++) {
            modal->modes[i].params.shape = character->mode_shape[i];
        }
    }

    // Store the character data for use during excitation
    current_characters_[node_idx] = *character;
}

// ============================================================================
// Node Count Control
// ============================================================================

void NodeManager::setNodeCount(uint8_t count) {
    // Clamp to valid range
    if (count < 1) count = 1;
    if (count > NUM_NETWORK_NODES) count = NUM_NETWORK_NODES;

    // Stop all nodes before changing count (safety)
    allNotesOff();

    // Force reset nodes beyond active count to clear modal state
    // This prevents inactive nodes from retaining energy and participating in coupling
    for (uint8_t i = count; i < NUM_NETWORK_NODES; i++) {
        if (nodes_[i]) {
            nodes_[i]->reset();
        }
    }

    active_node_count_ = count;
}

void NodeManager::setGlobalDamping(float damping) {
    if (!initialized_) return;

    // Apply global damping to all nodes
    for (uint8_t i = 0; i < NUM_NETWORK_NODES; i++) {
        if (nodes_[i]) {
            nodes_[i]->setGlobalDamping(damping);
        }
    }
}

// ============================================================================
// Note Routing
// ============================================================================

uint8_t NodeManager::routeNoteToNodes(uint8_t midi_note, uint8_t midi_channel, uint8_t* target_nodes) {
    switch (routing_mode_) {
        case NoteRoutingMode::MidiChannel:
            // Route by MIDI channel: Channel 1→Node 0, Channel 2→Node 1, etc.
            // midi_channel is 0-based (0 = channel 1)
            {
                uint8_t node_idx = midi_channel % active_node_count_;
                target_nodes[0] = node_idx;
                return 1;  // One node
            }

        case NoteRoutingMode::AllNodes:
            // All active nodes receive the note
            {
                for (uint8_t i = 0; i < active_node_count_; i++) {
                    target_nodes[i] = i;
                }
                return active_node_count_;
            }

        default:
            target_nodes[0] = 0;
            return 1;
    }
}

// ============================================================================
// Note Handling
// ============================================================================

void NodeManager::noteOn(uint8_t midi_note, float velocity, uint8_t midi_channel) {
    if (!initialized_ || midi_note > 127) return;

    // Route to target node(s)
    uint8_t target_nodes[NUM_NETWORK_NODES];
    uint8_t num_targets = routeNoteToNodes(midi_note, midi_channel, target_nodes);

    // Excite each target node
    for (uint8_t i = 0; i < num_targets; i++) {
        uint8_t node_idx = target_nodes[i];

        // Check multi-excite mode
        bool node_is_active = nodes_[node_idx]->isActive();

        if (multi_excite_mode_ == MultiExciteMode::ReTrigger && node_is_active) {
            // Re-trigger: reset node first
            nodes_[node_idx]->reset();
        }
        // If Accumulate mode: just excite on top of existing state

        // Excite the node
        exciteNode(node_idx, midi_note, velocity);
    }

    // Track note → node mapping for note-off (use first target for simplicity)
    if (num_targets > 0) {
        note_to_node_[midi_note] = target_nodes[0];
        note_to_channel_[midi_note] = midi_channel;
    }
}

void NodeManager::noteOff(uint8_t midi_note) {
    if (midi_note > 127) return;

    uint8_t node_idx = note_to_node_[midi_note];
    if (node_idx < NUM_NETWORK_NODES) {
        // Release node if it's still playing this note
        // (In accumulate mode, we only release if this was the last note)
        nodes_[node_idx]->noteOff();
        note_to_node_[midi_note] = 0xFF;
    }
}

void NodeManager::allNotesOff() {
    // Release all nodes
    for (uint8_t i = 0; i < NUM_NETWORK_NODES; i++) {
        if (nodes_[i]->isActive()) {
            nodes_[i]->noteOff();
        }
    }

    // Clear note mapping
    memset(note_to_node_, 0xFF, sizeof(note_to_node_));
}

void NodeManager::setPitchBend(float bend_amount) {
    pitch_bend_ = bend_amount;

    // Apply to all active nodes
    for (uint8_t i = 0; i < NUM_NETWORK_NODES; i++) {
        if (nodes_[i]->isActive()) {
            nodes_[i]->setPitchBend(bend_amount);
        }
    }
}

// ============================================================================
// Direct Node Access
// ============================================================================

ModalVoice* NodeManager::getNode(uint8_t node_idx) {
    if (node_idx >= NUM_NETWORK_NODES) return nullptr;
    return nodes_[node_idx];
}

void NodeManager::exciteNode(uint8_t node_idx, uint8_t midi_note, float velocity) {
    if (node_idx >= NUM_NETWORK_NODES) return;

    ModalVoice* node = nodes_[node_idx];
    const NodeCharacter* character = &current_characters_[node_idx];

    // Apply note-on with character's poke strength modulation
    float effective_velocity = velocity * character->poke_strength;
    node->noteOn(midi_note, effective_velocity);

    // Apply pitch bend
    node->setPitchBend(pitch_bend_);

    // Apply character's mode parameters
    // Get base frequency from the note
    float base_freq = node->getBaseFrequency();

    for (uint8_t mode_idx = 0; mode_idx < 4; mode_idx++) {
        float mode_freq = base_freq * character->mode_freq_mult[mode_idx];
        node->setMode(mode_idx, mode_freq,
                     character->mode_damping[mode_idx],
                     character->mode_weight[mode_idx]);
    }

    // Apply personality (in case it changed)
    node->setPersonality(character->personality);
}

void NodeManager::releaseNode(uint8_t node_idx) {
    if (node_idx >= NUM_NETWORK_NODES) return;
    nodes_[node_idx]->noteOff();
}

// ============================================================================
// Rendering
// ============================================================================

void NodeManager::updateNodes() {
    if (!initialized_) return;

    // Update only active nodes at control rate
    for (uint8_t i = 0; i < active_node_count_; i++) {
        if (nodes_[i]->isActive()) {
            nodes_[i]->updateModal();
        }
    }
}

void NodeManager::renderAudio(float* outL, float* outR, uint32_t num_frames) {
    if (!initialized_) {
        // Return silence
        memset(outL, 0, num_frames * sizeof(float));
        memset(outR, 0, num_frames * sizeof(float));
        return;
    }

    // Clear output buffers
    memset(outL, 0, num_frames * sizeof(float));
    memset(outR, 0, num_frames * sizeof(float));

    // Check buffer size
    if (num_frames > max_buffer_size_) {
        // Shouldn't happen, but handle gracefully
        num_frames = max_buffer_size_;
    }

    // OPTIMIZATION: Only render active node count
    // Skip nodes beyond active_node_count_ and inactive nodes
    for (uint8_t i = 0; i < active_node_count_; i++) {
        // Skip inactive nodes (major performance win!)
        if (!nodes_[i]->isActive()) {
            continue;
        }

        // Render active node to temp buffer
        nodes_[i]->renderAudio(temp_buffer_L_, temp_buffer_R_, num_frames);

        // Mix into output
        for (uint32_t j = 0; j < num_frames; j++) {
            outL[j] += temp_buffer_L_[j];
            outR[j] += temp_buffer_R_[j];
        }
    }
}

// ============================================================================
// Status
// ============================================================================

uint32_t NodeManager::getActiveNodeCount() const {
    uint32_t count = 0;
    for (uint8_t i = 0; i < NUM_NETWORK_NODES; i++) {
        if (nodes_[i]->isActive()) {
            count++;
        }
    }
    return count;
}

bool NodeManager::isNodeActive(uint8_t node_idx) const {
    if (node_idx >= NUM_NETWORK_NODES) return false;
    return nodes_[node_idx]->isActive();
}
