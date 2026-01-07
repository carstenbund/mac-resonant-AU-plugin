//
//  ModalAttractorsExtensionParameterAddresses.h
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/7/26.
//

#pragma once

#include <AudioToolbox/AUParameters.h>

// Match the parameter IDs from ModalParameters.h
typedef NS_ENUM(AUParameterAddress, ModalAttractorsExtensionParameterAddress) {
    // Global parameters
    kParam_MasterGain = 0,
    kParam_CouplingStrength = 1,
    kParam_Topology = 2,

    // Per-mode parameters (Mode 0)
    kParam_Mode0_Frequency = 3,
    kParam_Mode0_Damping = 4,
    kParam_Mode0_Weight = 5,

    // Per-mode parameters (Mode 1)
    kParam_Mode1_Frequency = 6,
    kParam_Mode1_Damping = 7,
    kParam_Mode1_Weight = 8,

    // Per-mode parameters (Mode 2)
    kParam_Mode2_Frequency = 9,
    kParam_Mode2_Damping = 10,
    kParam_Mode2_Weight = 11,

    // Per-mode parameters (Mode 3)
    kParam_Mode3_Frequency = 12,
    kParam_Mode3_Damping = 13,
    kParam_Mode3_Weight = 14,

    // Excitation parameters
    kParam_PokeStrength = 15,
    kParam_PokeDuration = 16,

    // Voice parameters
    kParam_Polyphony = 17,
    kParam_Personality = 18
};
