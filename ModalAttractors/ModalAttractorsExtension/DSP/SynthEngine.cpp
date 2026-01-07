/**
 * @file SynthEngine.cpp
 * @brief Implementation of the Modal Attractors synthesis engine
 */

#include "SynthEngine.h"
#include "VoiceAllocator.h"
#include "TopologyEngine.h"
#include "ModalVoice.h"
#include <algorithm>

// Parameter IDs (should match ModalParameters.h)
enum ParamID {
    kParam_MasterGain = 0,
    kParam_CouplingStrength,
    kParam_Topology
};

SynthEngine::SynthEngine(uint32_t maxPolyphony)
    : voiceAllocator_(nullptr)
    , topologyEngine_(nullptr)
    , voicePointers_(nullptr)
    , maxPolyphony_(maxPolyphony)
    , sampleRate_(44100.0)
    , maxFrames_(0)
    , channels_(2)
    , initialized_(false)
    , controlRateCounter_(0)
    , masterGain_(0.7f)
    , couplingStrength_(0.3f)
    , topologyType_(0)
{
    // Allocate DSP components (done once at construction)
    voiceAllocator_ = new VoiceAllocator(maxPolyphony);
    topologyEngine_ = new TopologyEngine(maxPolyphony);

    // Pre-allocate voice pointer array (CRITICAL: no allocation in render!)
    voicePointers_ = new ModalVoice*[maxPolyphony];
}

SynthEngine::~SynthEngine() {
    if (voicePointers_) {
        delete[] voicePointers_;
        voicePointers_ = nullptr;
    }

    if (voiceAllocator_) {
        delete voiceAllocator_;
        voiceAllocator_ = nullptr;
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

    // Initialize voice allocator
    voiceAllocator_->initialize(static_cast<float>(sampleRate));

    // Set default topology
    topologyEngine_->generateTopology(TopologyType::Ring, couplingStrength_);

    initialized_ = true;
}

void SynthEngine::reset() {
    if (!initialized_) return;

    // Release all voices
    voiceAllocator_->allNotesOff();
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
            voiceAllocator_->noteOn(event.noteOn.note, event.noteOn.velocity);
            break;

        case EventType::NoteOff:
            voiceAllocator_->noteOff(event.noteOff.note);
            break;

        case EventType::PitchBend:
            voiceAllocator_->setPitchBend(event.pitchBend.value);
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

    // Render voices
    voiceAllocator_->renderAudio(outL, outR, numFrames);

    // Apply master gain
    for (uint32_t i = 0; i < numFrames; i++) {
        outL[i] *= masterGain_;
        if (outR != outL) {
            outR[i] *= masterGain_;
        }
    }
}

void SynthEngine::updateControlRate() {
    // Update voice state at control rate
    voiceAllocator_->updateVoices();

    // Update coupling (FIXED: no allocation, use pre-allocated array)
    for (uint32_t i = 0; i < maxPolyphony_; i++) {
        voicePointers_[i] = voiceAllocator_->getVoice(i);
    }
    topologyEngine_->updateCoupling(voicePointers_, maxPolyphony_);
}

void SynthEngine::setParameter(uint32_t paramId, float value) {
    switch (paramId) {
        case kParam_MasterGain:
            masterGain_ = value;
            break;

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

        // TODO: Add per-mode parameters, poke strength, etc.

        default:
            break;
    }
}

float SynthEngine::getParameter(uint32_t paramId) const {
    switch (paramId) {
        case kParam_MasterGain:
            return masterGain_;

        case kParam_CouplingStrength:
            return couplingStrength_;

        case kParam_Topology:
            return static_cast<float>(topologyType_);

        default:
            return 0.0f;
    }
}
