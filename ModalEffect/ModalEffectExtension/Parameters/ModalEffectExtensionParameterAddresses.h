//
//  ModalEffectExtensionParameterAddresses.h
//  ModalEffectExtension
//
//  Resonant Body Effect Parameters
//

#pragma once

#include <AudioToolbox/AUParameters.h>

// ModalEffect - Resonant Body Effect Parameters
typedef NS_ENUM(AUParameterAddress, ModalEffectExtensionParameterAddress) {
    // Effect parameters
    kParam_BodySize = 0,     // Body size [0, 1] - scales resonator frequencies
    kParam_Material = 1,     // Material hardness [0, 1] - controls damping
    kParam_Excite = 2,       // Excitation amount [0, 1] - input drive
    kParam_Morph = 3,        // Pitch tracking amount [0, 1] - morphing
    kParam_Mix = 4           // Dry/wet mix [0, 1] - effect blend
};
