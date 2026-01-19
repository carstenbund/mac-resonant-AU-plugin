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
    kParam_MultiExcite = 26,      // Multi-excite: 0=ReTrigger, 1=Accumulate

    // Wave Shape Selection (20 parameters: 5 nodes × 4 modes)
    // Node 0 wave shapes
    kParam_Node0_Mode0_WaveShape = 27,
    kParam_Node0_Mode1_WaveShape = 28,
    kParam_Node0_Mode2_WaveShape = 29,
    kParam_Node0_Mode3_WaveShape = 30,

    // Node 1 wave shapes
    kParam_Node1_Mode0_WaveShape = 31,
    kParam_Node1_Mode1_WaveShape = 32,
    kParam_Node1_Mode2_WaveShape = 33,
    kParam_Node1_Mode3_WaveShape = 34,

    // Node 2 wave shapes
    kParam_Node2_Mode0_WaveShape = 35,
    kParam_Node2_Mode1_WaveShape = 36,
    kParam_Node2_Mode2_WaveShape = 37,
    kParam_Node2_Mode3_WaveShape = 38,

    // Node 3 wave shapes
    kParam_Node3_Mode0_WaveShape = 39,
    kParam_Node3_Mode1_WaveShape = 40,
    kParam_Node3_Mode2_WaveShape = 41,
    kParam_Node3_Mode3_WaveShape = 42,

    // Node 4 wave shapes
    kParam_Node4_Mode0_WaveShape = 43,
    kParam_Node4_Mode1_WaveShape = 44,
    kParam_Node4_Mode2_WaveShape = 45,
    kParam_Node4_Mode3_WaveShape = 46,

    // Character Editor state - tracks which node is being edited (0-4)
    // Hidden parameter set by UI when user selects a node in Character Editor
    kParam_EditingNodeIndex = 47
};
