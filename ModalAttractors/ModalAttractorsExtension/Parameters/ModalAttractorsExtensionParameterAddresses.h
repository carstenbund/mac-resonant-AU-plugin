//
//  ModalAttractorsExtensionParameterAddresses.h
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/7/26.
//

#pragma once

#include <AudioToolbox/AUParameters.h>

// Match the parameter IDs from ModalParameters.h
// NOTE: Node-based architecture - each "voice" is actually a NODE containing 4 internal modes
typedef NS_ENUM(AUParameterAddress, ModalAttractorsExtensionParameterAddress) {
    // Global parameters
    kParam_MasterGain = 0,
    kParam_CouplingStrength = 1,
    kParam_Topology = 2,
    kParam_NodeCount = 3,  // NEW: Number of active nodes (1-16)

    // Per-mode parameters (Mode 0) - SHIFTED by +1
    kParam_Mode0_Frequency = 4,
    kParam_Mode0_Damping = 5,
    kParam_Mode0_Weight = 6,

    // Per-mode parameters (Mode 1)
    kParam_Mode1_Frequency = 7,
    kParam_Mode1_Damping = 8,
    kParam_Mode1_Weight = 9,

    // Per-mode parameters (Mode 2)
    kParam_Mode2_Frequency = 10,
    kParam_Mode2_Damping = 11,
    kParam_Mode2_Weight = 12,

    // Per-mode parameters (Mode 3)
    kParam_Mode3_Frequency = 13,
    kParam_Mode3_Damping = 14,
    kParam_Mode3_Weight = 15,

    // Excitation parameters
    kParam_PokeStrength = 16,
    kParam_PokeDuration = 17,

    // Voice/Node parameters (deprecated - kept for compatibility)
    kParam_Polyphony = 18,    // Read-only, always 16 (max nodes)
    kParam_Personality = 19
};
