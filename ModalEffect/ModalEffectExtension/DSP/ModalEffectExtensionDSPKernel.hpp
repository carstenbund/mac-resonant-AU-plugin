//
//  ModalEffectExtensionDSPKernel.hpp
//  ModalEffectExtension
//
//  Created by Carsten on 1/7/26.
//

#pragma once

#import <AudioToolbox/AudioToolbox.h>
#import <CoreMIDI/MIDIMessages.h>

#import <algorithm>
#import <vector>

#import "ModalEffectExtensionParameterAddresses.h"

/*
 ModalEffectExtensionDSPKernel
 As a non-ObjC class, this is safe to use from render thread.
 */
class ModalEffectExtensionDSPKernel {
public:
    void initialize(double inSampleRate) {
        mSampleRate = inSampleRate;
    }
    
    void deInitialize() {
    }
    
    // MARK: - Bypass
    bool isBypassed() {
        return mBypassed;
    }
    
    void setBypass(bool shouldBypass) {
        mBypassed = shouldBypass;
    }
    
    // MARK: - Parameter Getter / Setter
    // Add a case for each parameter in ModalEffectExtensionParameterAddresses.h
    void setParameter(AUParameterAddress address, AUValue value) {
        switch (address) {
            case ModalEffectExtensionParameterAddress::kParam_BodySize:
                // TODO: Implement parameter handling
                break;
            case ModalEffectExtensionParameterAddress::kParam_Material:
                // TODO: Implement parameter handling
                break;
            case ModalEffectExtensionParameterAddress::kParam_Excite:
                // TODO: Implement parameter handling
                break;
            case ModalEffectExtensionParameterAddress::kParam_Morph:
                // TODO: Implement parameter handling
                break;
            case ModalEffectExtensionParameterAddress::kParam_Mix:
                // TODO: Implement parameter handling
                break;
            default:
                break;
        }
    }

    AUValue getParameter(AUParameterAddress address) {
        // Return the goal. It is not thread safe to return the ramping value.

        switch (address) {
            case ModalEffectExtensionParameterAddress::kParam_BodySize:
                return 0.5f; // TODO: Return actual value

            case ModalEffectExtensionParameterAddress::kParam_Material:
                return 0.5f; // TODO: Return actual value

            case ModalEffectExtensionParameterAddress::kParam_Excite:
                return 0.5f; // TODO: Return actual value

            case ModalEffectExtensionParameterAddress::kParam_Morph:
                return 0.0f; // TODO: Return actual value

            case ModalEffectExtensionParameterAddress::kParam_Mix:
                return 0.5f; // TODO: Return actual value

            default: return 0.f;
        }
    }
    
    // MARK: - Maximum Frames To Render
    AUAudioFrameCount maximumFramesToRender() const {
        return mMaxFramesToRender;
    }
    
    void setMaximumFramesToRender(const AUAudioFrameCount &maxFrames) {
        mMaxFramesToRender = maxFrames;
    }
    
    // MARK: - Musical Context
    void setMusicalContextBlock(AUHostMusicalContextBlock contextBlock) {
        mMusicalContextBlock = contextBlock;
    }
    
    // MARK: - MIDI Output
    void setMIDIOutputEventBlock(AUMIDIEventListBlock midiOutBlock) {
        mMIDIOutBlock = midiOutBlock;
    }
    
    // MARK: - MIDI Protocol
    MIDIProtocolID AudioUnitMIDIProtocol() const {
        return kMIDIProtocol_2_0;
    }
    
    /**
     MARK: - Internal Process

     This function does the core signal processing.
     TODO: Integrate actual Modal Attractors DSP engine.
     */
    void process(AUEventSampleTime bufferStartTime, AUAudioFrameCount frameCount) {

        if (mBypassed) { return; }

        // Use this to get Musical context info from the Plugin Host,
        // Replace nullptr with &memberVariable according to the AUHostMusicalContextBlock function signature
        if (mMusicalContextBlock) {
            mMusicalContextBlock(nullptr /* currentTempo */,
                                 nullptr /* timeSignatureNumerator */,
                                 nullptr /* timeSignatureDenominator */,
                                 nullptr /* currentBeatPosition */,
                                 nullptr /* sampleOffsetToNextBeat */,
                                 nullptr /* currentMeasureDownbeatPosition */);
        }

        // TODO: Process modal synthesis audio here

    }
    
    void handleOneEvent(AUEventSampleTime now, AURenderEvent const *event) {
        switch (event->head.eventType) {
            case AURenderEventParameter: {
                handleParameterEvent(now, event->parameter);
                break;
            }
                
            case AURenderEventMIDIEventList: {
                handleMIDIEventList(now, &event->MIDIEventsList);
                break;
            }
                
            default:
                break;
        }
    }

    void handleMIDIEventList(AUEventSampleTime now, AUMIDIEventList const* midiEvent) {
        /*
         // Parse UMP messages
         auto visitor = [] (void* context, MIDITimeStamp timeStamp, MIDIUniversalMessage message) {
         auto thisObject = static_cast<ModalEffectExtensionDSPKernel *>(context);

         switch (message.type) {
         case kMIDIMessageTypeChannelVoice2: {
         }
         break;

         default:
         break;
         }
         };
         MIDIEventListForEachEvent(&midiEvent->eventList, visitor, this);
         */
        if (mMIDIOutBlock)
        {
            mMIDIOutBlock(now, 0, &midiEvent->eventList);
        }
    }
    
    void handleParameterEvent(AUEventSampleTime now, AUParameterEvent const& parameterEvent) {
        // Implement handling incoming Parameter events as needed
    }
    
    // MARK: Member Variables
    AUHostMusicalContextBlock mMusicalContextBlock;

    double mSampleRate = 44100.0;
    bool mBypassed = false;
    AUAudioFrameCount mMaxFramesToRender = 1024;

    AUMIDIEventListBlock mMIDIOutBlock;

    // TODO: Add Modal Attractors DSP engine instance
};
