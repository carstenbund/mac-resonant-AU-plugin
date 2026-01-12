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
#include <algorithm>

// Parameter IDs for ModalEffect (must match ModalEffectExtensionParameterAddresses.h)
enum ParamID {
    kParam_BodySize = 0,     // Body size [0, 1] - scales resonator frequencies
    kParam_Material = 1,     // Material hardness [0, 1] - controls damping
    kParam_Excite = 2,       // Excitation amount [0, 1] - input drive
    kParam_Morph = 3,        // Pitch tracking amount [0, 1] - morphing
    kParam_Mix = 4           // Dry/wet mix [0, 1] - effect blend
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
    // ModalEffect parameters (5 total)
    , bodySize_(0.5f)      // Default body size
    , material_(0.5f)       // Default material
    , excite_(0.5f)         // Default excitation
    , morph_(0.0f)          // Default morph (no pitch tracking)
    , mix_(0.5f)            // Default mix (50% wet)
    // Legacy parameters kept for compatibility
    , masterGain_(0.7f)
    , couplingStrength_(0.3f)
    , topologyType_(0)
    , couplingMode_(ModalVoice::CouplingMode::ComplexDiffusion)
    , node0_character_(0)
    , node1_character_(1)
    , node2_character_(2)
    , node3_character_(3)
    , node4_character_(4)
    , noteRouting_(0)
    , multiExcite_(1)
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
    // Excitation parameters (for Character Editor)
    , pokeStrength_(0.5f)
    , pokeDuration_(10.0f)
    // Deprecated
    , personality_(0.0f)
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
        // ModalEffect parameters
        case kParam_BodySize:
            bodySize_ = value;
            break;

        case kParam_Material:
            material_ = value;
            break;

        case kParam_Excite:
            excite_ = value;
            break;

        case kParam_Morph:
            morph_ = value;
            break;

        case kParam_Mix:
            mix_ = value;
            break;

        default:
            // Unknown parameter - ignore
            break;
    }
}

float SynthEngine::getParameter(uint32_t paramId) const {
    switch (paramId) {
        // ModalEffect parameters
        case kParam_BodySize:
            return bodySize_;
        case kParam_Material:
            return material_;
        case kParam_Excite:
            return excite_;
        case kParam_Morph:
            return morph_;
        case kParam_Mix:
            return mix_;

        default:
            // Unknown parameter
            return 0.0f;
    }
}
