//
//  ModalAttractorsExtensionParameterAddresses.h
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/7/26.
//

#pragma once

#include <AudioToolbox/AUParameters.h>

typedef NS_ENUM(AUParameterAddress, ModalAttractorsExtensionParameterAddress) {
    sendNote = 0,
    midiNoteNumber = 1
};
