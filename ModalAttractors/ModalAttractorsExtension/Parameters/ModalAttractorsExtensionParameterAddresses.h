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
    param_MasterGain = 0,
    param_CouplingStrength = 1,
    param_Topology = 2,
    param_NodeCount = 3,  // NEW: Number of active nodes (1-16)

    // Per-mode parameters (Mode 0) - SHIFTED by +1
    param_Mode0_Frequency = 4,
    param_Mode0_Damping = 5,
    param_Mode0_Weight = 6,

    // Per-mode parameters (Mode 1)
    param_Mode1_Frequency = 7,
    param_Mode1_Damping = 8,
    param_Mode1_Weight = 9,

    // Per-mode parameters (Mode 2)
    param_Mode2_Frequency = 10,
    param_Mode2_Damping = 11,
    param_Mode2_Weight = 12,

    // Per-mode parameters (Mode 3)
    param_Mode3_Frequency = 13,
    param_Mode3_Damping = 14,
    param_Mode3_Weight = 15,

    // Excitation parameters
    param_PokeStrength = 16,
    param_PokeDuration = 17,

    // Voice/Node parameters (deprecated - kept for compatibility)
    param_Polyphony = 18,    // Read-only, always 16 (max nodes)
    param_Personality = 19,

    // ADSR Envelope parameters
    param_Attack = 20,
    param_Decay = 21,
    param_Sustain = 22,
    param_Release = 23
};
