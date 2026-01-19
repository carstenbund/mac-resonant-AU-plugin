/**
 * @file SynthEngine.cpp
 * @brief Implementation of the Modal Attractors synthesis engine
 *
 * Updated for Node Character System - 5 fixed nodes with selectable characters
 */

#include "SynthEngine.h"
#include "NodeManager.h"
#include "TopologyEngine.h"
#include "ModalVoice.h"
#include "NodeCharacter.h"
#include <algorithm>

// Parameter IDs (should match ModalAttractorsExtensionParameterAddresses.h)
enum ParamID {
    // Global
    kParam_MasterGain = 0,
    kParam_CouplingStrength = 1,
    kParam_Topology = 2,
    kParam_NodeCount = 3,  // Deprecated: always 5

    // Mode parameters (for Character Editor)
    kParam_Mode0_Frequency = 4,
    kParam_Mode0_Damping = 5,
    kParam_Mode0_Weight = 6,
    kParam_Mode1_Frequency = 7,
    kParam_Mode1_Damping = 8,
    kParam_Mode1_Weight = 9,
    kParam_Mode2_Frequency = 10,
    kParam_Mode2_Damping = 11,
    kParam_Mode2_Weight = 12,
    kParam_Mode3_Frequency = 13,
    kParam_Mode3_Damping = 14,
    kParam_Mode3_Weight = 15,
    kParam_PokeStrength = 16,
    kParam_PokeDuration = 17,

    // Deprecated
    kParam_Polyphony = 18,
    kParam_Personality = 19,

    // Node Character System
    kParam_Node0_Character = 20,
    kParam_Node1_Character = 21,
    kParam_Node2_Character = 22,
    kParam_Node3_Character = 23,
    kParam_Node4_Character = 24,
    kParam_NoteRouting = 25,
    kParam_MultiExcite = 26,
    // Wave Shape Selection (20 parameters: 5 nodes × 4 modes)
    kParam_Node0_Mode0_WaveShape = 27,
    kParam_Node0_Mode1_WaveShape = 28,
    kParam_Node0_Mode2_WaveShape = 29,
    kParam_Node0_Mode3_WaveShape = 30,
    kParam_Node1_Mode0_WaveShape = 31,
    kParam_Node1_Mode1_WaveShape = 32,
    kParam_Node1_Mode2_WaveShape = 33,
    kParam_Node1_Mode3_WaveShape = 34,
    kParam_Node2_Mode0_WaveShape = 35,
    kParam_Node2_Mode1_WaveShape = 36,
    kParam_Node2_Mode2_WaveShape = 37,
    kParam_Node2_Mode3_WaveShape = 38,
    kParam_Node3_Mode0_WaveShape = 39,
    kParam_Node3_Mode1_WaveShape = 40,
    kParam_Node3_Mode2_WaveShape = 41,
    kParam_Node3_Mode3_WaveShape = 42,
    kParam_Node4_Mode0_WaveShape = 43,
    kParam_Node4_Mode1_WaveShape = 44,
    kParam_Node4_Mode2_WaveShape = 45,
    kParam_Node4_Mode3_WaveShape = 46,

    // Character Editor - currently editing node index (0-4)
    // This hidden parameter tracks which node is selected in the Character Editor
    // When mode/excitation/personality parameters change, they're applied to this node
    kParam_EditingNodeIndex = 47
};

SynthEngine::SynthEngine(uint32_t maxPolyphony)
    : nodeManager_(nullptr)
    , topologyEngine_(nullptr)
    , nodePointers_(nullptr)
    , sampleRate_(44100.0)
    , maxFrames_(0)
    , channels_(2)
    , initialized_(false)
    , controlRateCounter_(0)
    // Global parameters
    , masterGain_(0.7f)
    , couplingStrength_(0.3f)
    , topologyType_(0)
    , couplingMode_(ModalVoice::CouplingMode::ComplexDiffusion)  // Testing new phase-preserving coupling
    // Node characters (default: each node gets its own character 0-4)
    , node0_character_(0)
    , node1_character_(1)
    , node2_character_(2)
    , node3_character_(3)
    , node4_character_(4)
    // Routing
    , noteRouting_(0)      // RoundRobin
    , multiExcite_(1)      // Accumulate
    // Mode parameters (for Character Editor)
    , mode0_frequency_(1.0f)
    , mode0_damping_(1.0f)
    , mode0_weight_(1.0f)
    , mode1_frequency_(2.0f)
    , mode1_damping_(1.2f)
    , mode1_weight_(0.8f)
    , mode2_frequency_(3.0f)
    , mode2_damping_(1.5f)
    , mode2_weight_(0.6f)
    , mode3_frequency_(4.5f)
    , mode3_damping_(2.0f)
    , mode3_weight_(0.4f)
    , mode4_frequency_(5.5f)
    , mode4_damping_(2.5f)
    , mode4_weight_(0.3f)
    , mode5_frequency_(6.5f)
    , mode5_damping_(3.0f)
    , mode5_weight_(0.2f)
    // Excitation parameters (for Character Editor)
    , pokeStrength_(0.5f)
    , pokeDuration_(10.0f)
    // Deprecated
    , personality_(0.0f)
    // Character Editor state
    , editingNodeIndex_(0)
{
    // Allocate DSP components (always 5 nodes)
    nodeManager_ = new NodeManager();
    topologyEngine_ = new TopologyEngine(5);  // Fixed 5 nodes

    // Pre-allocate node pointer array (no allocation in render!)
    nodePointers_ = new ModalVoice*[5];
}

SynthEngine::~SynthEngine() {
    if (nodePointers_) {
        delete[] nodePointers_;
        nodePointers_ = nullptr;
    }

    if (nodeManager_) {
        delete nodeManager_;
        nodeManager_ = nullptr;
    }

    if (topologyEngine_) {
        delete topologyEngine_;
        topologyEngine_ = nullptr;
    }
}

void SynthEngine::prepare(double sampleRate, uint32_t maxFrames, uint32_t channels) {
    sampleRate_ = sampleRate;
    maxFrames_ = maxFrames;
    channels_ = channels;

    // Initialize node manager
    nodeManager_->initialize(static_cast<float>(sampleRate));

    // Apply default characters to all nodes
    for (uint8_t i = 0; i < 5; i++) {
        nodeManager_->setNodeCharacter(i, i);  // Node i gets character i
    }

    // Set default topology
    topologyEngine_->generateTopology(TopologyType::Ring, couplingStrength_);

    // Initialize global damping from volume control (0.0-1.0 range → 1.0-0.0 damping)
    float global_damping = 1.0f - masterGain_;
    nodeManager_->setGlobalDamping(global_damping);

    initialized_ = true;
}

void SynthEngine::reset() {
    if (!initialized_) return;

    // Release all nodes
    nodeManager_->allNotesOff();
    controlRateCounter_ = 0;
}

void SynthEngine::render(const EventQueue& events, float* outL, float* outR, uint32_t numFrames) {
    if (!initialized_) {
        // Return silence
        memset(outL, 0, numFrames * sizeof(float));
        if (outR != outL) {
            memset(outR, 0, numFrames * sizeof(float));
        }
        return;
    }

    // Sample-accurate event processing pattern:
    // Process events in order, rendering slices between events

    uint32_t lastOffset = 0;

    for (uint32_t i = 0; i < events.count(); i++) {
        const SynthEvent& event = events[i];
        int32_t offset = event.sampleOffset;

        // Clamp offset to valid range
        if (offset < 0) offset = 0;
        if (offset > static_cast<int32_t>(numFrames)) offset = numFrames;

        // Render slice before this event
        if (offset > static_cast<int32_t>(lastOffset)) {
            uint32_t sliceFrames = offset - lastOffset;
            renderSlice(outL + lastOffset, outR + lastOffset, lastOffset, sliceFrames);
        }

        // Process event at this sample offset
        processEvent(event);

        lastOffset = offset;
    }

    // Render remaining frames after all events
    if (lastOffset < numFrames) {
        uint32_t remainingFrames = numFrames - lastOffset;
        renderSlice(outL + lastOffset, outR + lastOffset, lastOffset, remainingFrames);
    }
}

void SynthEngine::processEvent(const SynthEvent& event) {
    switch (event.type) {
        case EventType::NoteOn:
            nodeManager_->noteOn(event.noteOn.note, event.noteOn.velocity, event.noteOn.channel);
            break;

        case EventType::NoteOff:
            nodeManager_->noteOff(event.noteOff.note);
            break;

        case EventType::PitchBend:
            nodeManager_->setPitchBend(event.pitchBend.value);
            break;

        case EventType::CC:
            // Handle CC messages if needed
            // Could map to parameters
            break;

        case EventType::Parameter:
            setParameter(event.parameter.paramId, event.parameter.value);
            break;
    }
}

void SynthEngine::renderSlice(float* outL, float* outR, uint32_t startFrame, uint32_t numFrames) {
    // Update control-rate parameters periodically
    controlRateCounter_ += numFrames;
    if (controlRateCounter_ >= CONTROL_RATE_SAMPLES) {
        updateControlRate();
        controlRateCounter_ = 0;
    }

    // Render nodes
    nodeManager_->renderAudio(outL, outR, numFrames);

    // Note: Volume control now functions as global damping (circuit energy control)
    // Output level is controlled in the DAW, not here
}

void SynthEngine::updateControlRate() {
    // Update node state at control rate
    nodeManager_->updateNodes();

    // Update coupling (fixed 5 nodes)
    for (uint8_t i = 0; i < 5; i++) {
        nodePointers_[i] = nodeManager_->getNode(i);
    }

    // Choose coupling method based on mode
    switch (couplingMode_) {
        case ModalVoice::CouplingMode::ComplexDiffusion:
            topologyEngine_->updateCouplingComplex(nodePointers_, 5);
            break;
        case ModalVoice::CouplingMode::MagnitudePressure:
        default:
            topologyEngine_->updateCoupling(nodePointers_, 5);
            break;
    }
}

void SynthEngine::setParameter(uint32_t paramId, float value) {
    switch (paramId) {
        case kParam_MasterGain: {
            masterGain_ = value;
            // Convert volume (0.0-1.0) to global damping (1.0-0.0)
            // Volume = 1.0 → damping = 0.0 (no damping, full resonance)
            // Volume = 0.5 → damping = 0.5 (moderate damping)
            // Volume = 0.0 → damping = 1.0 (maximum damping, fast cooldown)
            float global_damping = 1.0f - value;
            if (nodeManager_) {
                nodeManager_->setGlobalDamping(global_damping);
            }
            break;
        }

        case kParam_CouplingStrength:
            couplingStrength_ = value;
            topologyEngine_->setCouplingStrength(value);
            break;

        case kParam_Topology: {
            topologyType_ = static_cast<int>(value);

            // Map parameter value to topology type
            TopologyType topo = TopologyType::Ring;
            switch (topologyType_) {
                case 0: topo = TopologyType::Ring; break;
                case 1: topo = TopologyType::SmallWorld; break;
                case 2: topo = TopologyType::Clustered; break;
                case 3: topo = TopologyType::HubSpoke; break;
                case 4: topo = TopologyType::Random; break;
                case 5: topo = TopologyType::Complete; break;
                case 6: topo = TopologyType::None; break;
            }

            topologyEngine_->generateTopology(topo, couplingStrength_);
            break;
        }

        case kParam_NodeCount:
            // Set active node count (1-5)
            if (nodeManager_) {
                nodeManager_->setNodeCount(static_cast<uint8_t>(value));
            }
            break;

        // Node Character parameters
        case kParam_Node0_Character:
            node0_character_ = static_cast<uint8_t>(value);
            if (nodeManager_) {
                nodeManager_->setNodeCharacter(0, node0_character_);
            }
            break;

        case kParam_Node1_Character:
            node1_character_ = static_cast<uint8_t>(value);
            if (nodeManager_) {
                nodeManager_->setNodeCharacter(1, node1_character_);
            }
            break;

        case kParam_Node2_Character:
            node2_character_ = static_cast<uint8_t>(value);
            if (nodeManager_) {
                nodeManager_->setNodeCharacter(2, node2_character_);
            }
            break;

        case kParam_Node3_Character:
            node3_character_ = static_cast<uint8_t>(value);
            if (nodeManager_) {
                nodeManager_->setNodeCharacter(3, node3_character_);
            }
            break;

        case kParam_Node4_Character:
            node4_character_ = static_cast<uint8_t>(value);
            if (nodeManager_) {
                nodeManager_->setNodeCharacter(4, node4_character_);
            }
            break;

        // Routing parameters
        case kParam_NoteRouting:
            noteRouting_ = static_cast<uint8_t>(value);
            if (nodeManager_) {
                nodeManager_->setRoutingMode(static_cast<NoteRoutingMode>(noteRouting_));
            }
            break;

        case kParam_MultiExcite:
            multiExcite_ = static_cast<uint8_t>(value);
            if (nodeManager_) {
                nodeManager_->setMultiExciteMode(static_cast<MultiExciteMode>(multiExcite_));
            }
            break;

        // Mode parameters (for Character Editor)
        // Apply immediately to the currently edited node
        case kParam_Mode0_Frequency:
            mode0_frequency_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_Mode0_Damping:
            mode0_damping_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_Mode0_Weight:
            mode0_weight_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;

        case kParam_Mode1_Frequency:
            mode1_frequency_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_Mode1_Damping:
            mode1_damping_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_Mode1_Weight:
            mode1_weight_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;

        case kParam_Mode2_Frequency:
            mode2_frequency_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_Mode2_Damping:
            mode2_damping_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_Mode2_Weight:
            mode2_weight_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;

        case kParam_Mode3_Frequency:
            mode3_frequency_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_Mode3_Damping:
            mode3_damping_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_Mode3_Weight:
            mode3_weight_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;

        case kParam_Mode4_Frequency:
            mode4_frequency_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_Mode4_Damping:
            mode4_damping_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_Mode4_Weight:
            mode4_weight_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;

        case kParam_Mode5_Frequency:
            mode5_frequency_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_Mode5_Damping:
            mode5_damping_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_Mode5_Weight:
            mode5_weight_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;

        // Excitation parameters (for Character Editor)
        // Apply immediately to the currently edited node
        case kParam_PokeStrength:
            pokeStrength_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;
        case kParam_PokeDuration:
            pokeDuration_ = value;
            applyCustomCharacterToNode(editingNodeIndex_);
            break;

        // Wave Shape parameters (30 parameters: 5 nodes × 6 modes)
        // Handle all wave shape parameters in a range check
        default:
            if (paramId >= kParam_Node0_Mode0_WaveShape && paramId <= kParam_Node4_Mode5_WaveShape) {
                uint32_t paramOffset = paramId - kParam_Node0_Mode0_WaveShape;
                uint32_t nodeIndex = paramOffset / 6;  // 0-4
                uint32_t modeIndex = paramOffset % 6;  // 0-5
                wave_shape_t shape = static_cast<wave_shape_t>(static_cast<int>(value));

                if (nodeManager_) {
                    nodeManager_->setModeWaveShape(nodeIndex, modeIndex, shape);

                    // Apply custom character to this node when wave shapes change
                    // This ensures mode parameters from Character Editor are applied to voices
                    applyCustomCharacterToNode(nodeIndex);
                }
            }
            // Note: Also handles deprecated parameters below
            break;
    }

    // Handle deprecated and special parameters separately (outside switch for clarity)
    if (paramId == kParam_Polyphony) {
        // Always 5 nodes
    } else if (paramId == kParam_Personality) {
        personality_ = value;
        // Apply immediately to the currently edited node
        applyCustomCharacterToNode(editingNodeIndex_);
    } else if (paramId == kParam_EditingNodeIndex) {
        // Update which node is being edited in the Character Editor
        editingNodeIndex_ = static_cast<uint8_t>(value);
        if (editingNodeIndex_ > 4) editingNodeIndex_ = 0;  // Safety clamp
    }
}

float SynthEngine::getParameter(uint32_t paramId) const {
    switch (paramId) {
        // Global parameters
        case kParam_MasterGain:
            return masterGain_;
        case kParam_CouplingStrength:
            return couplingStrength_;
        case kParam_Topology:
            return static_cast<float>(topologyType_);
        case kParam_NodeCount:
            return nodeManager_ ? static_cast<float>(nodeManager_->getNodeCount()) : 5.0f;

        // Node Character parameters
        case kParam_Node0_Character:
            return static_cast<float>(node0_character_);
        case kParam_Node1_Character:
            return static_cast<float>(node1_character_);
        case kParam_Node2_Character:
            return static_cast<float>(node2_character_);
        case kParam_Node3_Character:
            return static_cast<float>(node3_character_);
        case kParam_Node4_Character:
            return static_cast<float>(node4_character_);

        // Routing parameters
        case kParam_NoteRouting:
            return static_cast<float>(noteRouting_);
        case kParam_MultiExcite:
            return static_cast<float>(multiExcite_);

        // Mode parameters (for Character Editor)
        case kParam_Mode0_Frequency:
            return mode0_frequency_;
        case kParam_Mode0_Damping:
            return mode0_damping_;
        case kParam_Mode0_Weight:
            return mode0_weight_;

        case kParam_Mode1_Frequency:
            return mode1_frequency_;
        case kParam_Mode1_Damping:
            return mode1_damping_;
        case kParam_Mode1_Weight:
            return mode1_weight_;

        case kParam_Mode2_Frequency:
            return mode2_frequency_;
        case kParam_Mode2_Damping:
            return mode2_damping_;
        case kParam_Mode2_Weight:
            return mode2_weight_;

        case kParam_Mode3_Frequency:
            return mode3_frequency_;
        case kParam_Mode3_Damping:
            return mode3_damping_;
        case kParam_Mode3_Weight:
            return mode3_weight_;

        case kParam_Mode4_Frequency:
            return mode4_frequency_;
        case kParam_Mode4_Damping:
            return mode4_damping_;
        case kParam_Mode4_Weight:
            return mode4_weight_;

        case kParam_Mode5_Frequency:
            return mode5_frequency_;
        case kParam_Mode5_Damping:
            return mode5_damping_;
        case kParam_Mode5_Weight:
            return mode5_weight_;

        // Excitation parameters (for Character Editor)
        case kParam_PokeStrength:
            return pokeStrength_;
        case kParam_PokeDuration:
            return pokeDuration_;

        // Deprecated parameters
        case kParam_Polyphony:
            return 5.0f;  // Always 5 nodes
        case kParam_Personality:
            return personality_;

        // Character Editor state
        case kParam_EditingNodeIndex:
            return static_cast<float>(editingNodeIndex_);

        // Wave Shape parameters (20 parameters: 5 nodes × 4 modes)
        default:
            if (paramId >= kParam_Node0_Mode0_WaveShape && paramId <= kParam_Node4_Mode3_WaveShape) {
                uint32_t paramOffset = paramId - kParam_Node0_Mode0_WaveShape;
                uint32_t nodeIndex = paramOffset / 4;  // 0-4
                uint32_t modeIndex = paramOffset % 4;  // 0-3

                if (nodeManager_) {
                    wave_shape_t shape = nodeManager_->getModeWaveShape(nodeIndex, modeIndex);
                    return static_cast<float>(static_cast<int>(shape));
                }
            }
            return 0.0f;
    }
}

void SynthEngine::applyCustomCharacterToNode(uint8_t nodeIndex) {
    if (nodeIndex >= 5 || !nodeManager_) return;

    // Build a custom NodeCharacter from the Character Editor parameters
    NodeCharacter customChar;

    // Mode parameters (frequency multipliers, damping, weights)
    customChar.mode_freq_mult[0] = mode0_frequency_;
    customChar.mode_damping[0] = mode0_damping_;
    customChar.mode_weight[0] = mode0_weight_;

    customChar.mode_freq_mult[1] = mode1_frequency_;
    customChar.mode_damping[1] = mode1_damping_;
    customChar.mode_weight[1] = mode1_weight_;

    customChar.mode_freq_mult[2] = mode2_frequency_;
    customChar.mode_damping[2] = mode2_damping_;
    customChar.mode_weight[2] = mode2_weight_;

    customChar.mode_freq_mult[3] = mode3_frequency_;
    customChar.mode_damping[3] = mode3_damping_;
    customChar.mode_weight[3] = mode3_weight_;

    customChar.mode_freq_mult[4] = mode4_frequency_;
    customChar.mode_damping[4] = mode4_damping_;
    customChar.mode_weight[4] = mode4_weight_;

    customChar.mode_freq_mult[5] = mode5_frequency_;
    customChar.mode_damping[5] = mode5_damping_;
    customChar.mode_weight[5] = mode5_weight_;

    // Wave shapes - get current wave shapes for this node
    for (uint32_t mode = 0; mode < 6; mode++) {
        customChar.mode_shape[mode] = nodeManager_->getModeWaveShape(nodeIndex, mode);
    }

    // Voice behavior
    customChar.personality = static_cast<node_personality_t>(static_cast<int>(personality_));

    // Excitation parameters
    customChar.poke_strength = pokeStrength_;
    customChar.poke_duration_ms = pokeDuration_;

    // Network behavior (default neutral response)
    customChar.coupling_response_gain = 1.0f;

    // Metadata
    customChar.name = "Custom Character";
    customChar.description = "Character Editor custom parameters";

    // Apply the custom character to the node
    nodeManager_->setNodeCharacterCustom(nodeIndex, &customChar);
}
