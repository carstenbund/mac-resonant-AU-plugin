//
//  ModalAttractorsExtensionParameterAddresses.h
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/7/26.
//

#pragma once

#include <AudioToolbox/AUParameters.h>

// Match the parameter IDs from ModalParameters.h
// NOTE: Node Character System - 5 fixed nodes, each with selectable character
typedef NS_ENUM(AUParameterAddress, ModalAttractorsExtensionParameterAddress) {
    // Global parameters
    kParam_MasterGain = 0,
    kParam_CouplingStrength = 1,
    kParam_Topology = 2,
    kParam_NodeCount = 3,  // Deprecated: Always 5 nodes now

    // Per-mode parameters (Mode 0) - Used by Character Editor only
    kParam_Mode0_Frequency = 4,
    kParam_Mode0_Damping = 5,
    kParam_Mode0_Weight = 6,

    // Per-mode parameters (Mode 1) - Used by Character Editor only
    kParam_Mode1_Frequency = 7,
    kParam_Mode1_Damping = 8,
    kParam_Mode1_Weight = 9,

    // Per-mode parameters (Mode 2) - Used by Character Editor only
    kParam_Mode2_Frequency = 10,
    kParam_Mode2_Damping = 11,
    kParam_Mode2_Weight = 12,

    // Per-mode parameters (Mode 3) - Used by Character Editor only
    kParam_Mode3_Frequency = 13,
    kParam_Mode3_Damping = 14,
    kParam_Mode3_Weight = 15,

    // Excitation parameters - Used by Character Editor only
    kParam_PokeStrength = 16,
    kParam_PokeDuration = 17,

    // Voice/Node parameters (deprecated)
    kParam_Polyphony = 18,    // Deprecated: Always 5 nodes
    kParam_Personality = 19,  // Deprecated: Per-character now

    // Node Character System (NEW) - Main UI parameters
    kParam_Node0_Character = 20,  // Character ID for node 0 (0-4)
    kParam_Node1_Character = 21,  // Character ID for node 1 (0-4)
    kParam_Node2_Character = 22,  // Character ID for node 2 (0-4)
    kParam_Node3_Character = 23,  // Character ID for node 3 (0-4)
    kParam_Node4_Character = 24,  // Character ID for node 4 (0-4)

    // Routing and behavior
    kParam_NoteRouting = 25,      // Routing mode: 0=RoundRobin, 1=PitchZones
    kParam_MultiExcite = 26       // Multi-excite: 0=ReTrigger, 1=Accumulate
};
