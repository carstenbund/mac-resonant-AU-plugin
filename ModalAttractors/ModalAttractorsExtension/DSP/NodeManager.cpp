/**
 * @file NodeManager.cpp
 * @brief Implementation of 5-node network manager
 */

#include "NodeManager.h"
#include <cstring>
#include <algorithm>

NodeManager::NodeManager()
    : routing_mode_(NoteRoutingMode::RoundRobin)
    , multi_excite_mode_(MultiExciteMode::Accumulate)
    , round_robin_counter_(0)
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

    // Store the character data for use during excitation
    current_characters_[node_idx] = *character;
}

// ============================================================================
// Note Routing
// ============================================================================

uint8_t NodeManager::routeNoteToNode(uint8_t midi_note) {
    switch (routing_mode_) {
        case NoteRoutingMode::RoundRobin:
            // Simple round-robin: cycle through nodes
            {
                uint8_t node_idx = round_robin_counter_ % NUM_NETWORK_NODES;
                round_robin_counter_++;
                return node_idx;
            }

        case NoteRoutingMode::PitchZones:
            // Pitch-based zones: low notes → low nodes, high notes → high nodes
            // MIDI range 0-127 divided into 5 zones
            if (midi_note < 36) return 0;      // 0-35: Bass zone (C0-B2)
            else if (midi_note < 60) return 1; // 36-59: Low zone (C3-B4)
            else if (midi_note < 72) return 2; // 60-71: Mid zone (C5-B5)
            else if (midi_note < 96) return 3; // 72-95: High zone (C6-B7)
            else return 4;                      // 96-127: Top zone (C8+)

        default:
            return 0;
    }
}

// ============================================================================
// Note Handling
// ============================================================================

void NodeManager::noteOn(uint8_t midi_note, float velocity) {
    if (!initialized_ || midi_note > 127) return;

    // Route to target node
    uint8_t node_idx = routeNoteToNode(midi_note);

    // Check multi-excite mode
    bool node_is_active = nodes_[node_idx]->isActive();

    if (multi_excite_mode_ == MultiExciteMode::ReTrigger && node_is_active) {
        // Re-trigger: reset node first
        nodes_[node_idx]->reset();
    }
    // If Accumulate mode: just excite on top of existing state

    // Excite the node
    exciteNode(node_idx, midi_note, velocity);

    // Track note → node mapping for note-off
    note_to_node_[midi_note] = node_idx;
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

    // Update all nodes at control rate
    for (uint8_t i = 0; i < NUM_NETWORK_NODES; i++) {
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

    // Mix all nodes (both active and inactive nodes are rendered, but inactive = silence)
    for (uint8_t i = 0; i < NUM_NETWORK_NODES; i++) {
        // Render node to temp buffer
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
