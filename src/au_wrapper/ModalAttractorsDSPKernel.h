/**
 * @file ModalAttractorsDSPKernel.h
 * @brief Real-time DSP kernel for Modal Attractors AU
 *
 * This kernel wraps the C++ DSP engine and provides a real-time safe
 * interface for the AUAudioUnit render callback.
 */

#ifndef MODAL_ATTRACTORS_DSP_KERNEL_H
#define MODAL_ATTRACTORS_DSP_KERNEL_H

#include "ModalAttractorsAU.h"
#include <cstdint>

/**
 * @brief DSP Kernel for real-time audio processing
 *
 * This class is called from the real-time audio thread.
 * All methods must be real-time safe (no locks, allocations, etc).
 */
class ModalAttractorsDSPKernel {
public:
    ModalAttractorsDSPKernel();
    ~ModalAttractorsDSPKernel();

    /**
     * Initialize the DSP kernel
     * @param sampleRate Sample rate in Hz
     * @param numChannels Number of output channels (1 or 2)
     */
    void init(double sampleRate, uint32_t numChannels);

    /**
     * Clean up resources
     */
    void cleanup();

    /**
     * Render audio (real-time safe)
     * @param outL Left channel output buffer
     * @param outR Right channel output buffer
     * @param numFrames Number of frames to render
     */
    void render(float *outL, float *outR, uint32_t numFrames);

    /**
     * Handle MIDI message (real-time safe)
     * @param status MIDI status byte
     * @param data1 MIDI data byte 1
     * @param data2 MIDI data byte 2
     */
    void handleMIDI(uint8_t status, uint8_t data1, uint8_t data2);

    /**
     * Set parameter value (real-time safe)
     * @param address Parameter address
     * @param value Parameter value
     */
    void setParameter(uint32_t address, float value);

    /**
     * Get parameter value (real-time safe)
     * @param address Parameter address
     * @return Current parameter value
     */
    float getParameter(uint32_t address);

private:
    ModalAttractorsEngine engine_;
    bool initialized_;
};

#endif // MODAL_ATTRACTORS_DSP_KERNEL_H
