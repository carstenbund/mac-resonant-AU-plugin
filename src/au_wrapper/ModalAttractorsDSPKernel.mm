/**
 * @file ModalAttractorsDSPKernel.mm
 * @brief DSP Kernel implementation for Modal Attractors
 */

#include "ModalAttractorsDSPKernel.h"
#include "ModalParameters.h"
#include <cstring>

ModalAttractorsDSPKernel::ModalAttractorsDSPKernel()
    : initialized_(false) {
    memset(&engine_, 0, sizeof(engine_));
}

ModalAttractorsDSPKernel::~ModalAttractorsDSPKernel() {
    cleanup();
}

void ModalAttractorsDSPKernel::init(double sampleRate, uint32_t numChannels) {
    if (initialized_) {
        cleanup();
    }

    // Initialize engine with default polyphony
    modal_attractors_engine_init(&engine_, sampleRate, kPolyphony_Default);

    initialized_ = true;
}

void ModalAttractorsDSPKernel::cleanup() {
    if (initialized_) {
        modal_attractors_engine_cleanup(&engine_);
        initialized_ = false;
    }
}

void ModalAttractorsDSPKernel::render(float *outL, float *outR, uint32_t numFrames) {
    if (!initialized_) {
        // Return silence if not initialized
        memset(outL, 0, numFrames * sizeof(float));
        if (outR != outL) {
            memset(outR, 0, numFrames * sizeof(float));
        }
        return;
    }

    // Render audio from engine
    modal_attractors_engine_render(&engine_, outL, outR, numFrames);
}

void ModalAttractorsDSPKernel::handleMIDI(uint8_t status, uint8_t data1, uint8_t data2) {
    if (!initialized_) {
        return;
    }

    uint8_t statusType = status & 0xF0;
    // uint8_t channel = status & 0x0F;  // MIDI channel (unused for now)

    switch (statusType) {
        case 0x90:  // Note On
            if (data2 > 0) {  // velocity > 0
                modal_attractors_engine_note_on(&engine_, data1, data2);
            } else {
                // velocity = 0 is treated as note off
                modal_attractors_engine_note_off(&engine_, data1);
            }
            break;

        case 0x80:  // Note Off
            modal_attractors_engine_note_off(&engine_, data1);
            break;

        // TODO: Add support for other MIDI messages
        // 0xE0 - Pitch Bend
        // 0xB0 - Control Change
        // 0xD0 - Channel Pressure

        default:
            break;
    }
}

void ModalAttractorsDSPKernel::setParameter(uint32_t address, float value) {
    if (!initialized_) {
        return;
    }

    modal_attractors_engine_set_parameter(&engine_, address, value);
}

float ModalAttractorsDSPKernel::getParameter(uint32_t address) {
    if (!initialized_) {
        return 0.0f;
    }

    // Return cached parameter values from engine
    switch (address) {
        case kParam_MasterGain:
            return engine_.master_gain;

        case kParam_CouplingStrength:
            return engine_.coupling_strength;

        case kParam_Topology:
            return static_cast<float>(engine_.topology_type);

        case kParam_Polyphony:
            return static_cast<float>(engine_.max_polyphony);

        case kParam_WaveShape:
            return static_cast<float>(engine_.wave_shape);

        case kParam_PulseWidth:
            return engine_.pulse_width;

        default:
            return 0.0f;
    }
}
