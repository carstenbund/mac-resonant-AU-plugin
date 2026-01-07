//
//  Parameters.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/7/26.
//

import Foundation
import AudioToolbox

// Parameter tree specification for Modal Attractors synthesis engine
let ModalAttractorsExtensionParameterSpecs = ParameterTreeSpec {

    // MARK: - Global Parameters
    ParameterGroupSpec(identifier: "global", name: "Global") {
        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_MasterGain,
            identifier: "masterGain",
            name: "Master Gain",
            units: .linearGain,
            valueRange: 0.0...1.0,
            defaultValue: 0.7
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_CouplingStrength,
            identifier: "couplingStrength",
            name: "Coupling Strength",
            units: .generic,
            valueRange: 0.0...1.0,
            defaultValue: 0.3
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Topology,
            identifier: "topology",
            name: "Topology",
            units: .indexed,
            valueRange: 0...6,
            defaultValue: 0,
            valueStrings: ["Ring", "Small World", "Clustered", "Hub-Spoke", "Random", "Complete", "None"]
        )
    }

    // MARK: - Mode 0 Parameters
    ParameterGroupSpec(identifier: "mode0", name: "Mode 0") {
        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Mode0_Frequency,
            identifier: "mode0Frequency",
            name: "Frequency Multiplier",
            units: .generic,
            valueRange: 0.5...8.0,
            defaultValue: 1.0
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Mode0_Damping,
            identifier: "mode0Damping",
            name: "Damping",
            units: .generic,
            valueRange: 0.1...5.0,
            defaultValue: 1.0
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Mode0_Weight,
            identifier: "mode0Weight",
            name: "Weight",
            units: .generic,
            valueRange: 0.0...1.0,
            defaultValue: 1.0
        )
    }

    // MARK: - Mode 1 Parameters
    ParameterGroupSpec(identifier: "mode1", name: "Mode 1") {
        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Mode1_Frequency,
            identifier: "mode1Frequency",
            name: "Frequency Multiplier",
            units: .generic,
            valueRange: 0.5...8.0,
            defaultValue: 2.0
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Mode1_Damping,
            identifier: "mode1Damping",
            name: "Damping",
            units: .generic,
            valueRange: 0.1...5.0,
            defaultValue: 1.2
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Mode1_Weight,
            identifier: "mode1Weight",
            name: "Weight",
            units: .generic,
            valueRange: 0.0...1.0,
            defaultValue: 0.8
        )
    }

    // MARK: - Mode 2 Parameters
    ParameterGroupSpec(identifier: "mode2", name: "Mode 2") {
        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Mode2_Frequency,
            identifier: "mode2Frequency",
            name: "Frequency Multiplier",
            units: .generic,
            valueRange: 0.5...8.0,
            defaultValue: 3.0
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Mode2_Damping,
            identifier: "mode2Damping",
            name: "Damping",
            units: .generic,
            valueRange: 0.1...5.0,
            defaultValue: 1.5
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Mode2_Weight,
            identifier: "mode2Weight",
            name: "Weight",
            units: .generic,
            valueRange: 0.0...1.0,
            defaultValue: 0.6
        )
    }

    // MARK: - Mode 3 Parameters
    ParameterGroupSpec(identifier: "mode3", name: "Mode 3") {
        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Mode3_Frequency,
            identifier: "mode3Frequency",
            name: "Frequency Multiplier",
            units: .generic,
            valueRange: 0.5...8.0,
            defaultValue: 4.5
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Mode3_Damping,
            identifier: "mode3Damping",
            name: "Damping",
            units: .generic,
            valueRange: 0.1...5.0,
            defaultValue: 2.0
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Mode3_Weight,
            identifier: "mode3Weight",
            name: "Weight",
            units: .generic,
            valueRange: 0.0...1.0,
            defaultValue: 0.4
        )
    }

    // MARK: - Excitation Parameters
    ParameterGroupSpec(identifier: "excitation", name: "Excitation") {
        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_PokeStrength,
            identifier: "pokeStrength",
            name: "Poke Strength",
            units: .generic,
            valueRange: 0.0...1.0,
            defaultValue: 0.5
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_PokeDuration,
            identifier: "pokeDuration",
            name: "Poke Duration",
            units: .milliseconds,
            valueRange: 1.0...50.0,
            defaultValue: 10.0
        )
    }

    // MARK: - Voice Parameters
    ParameterGroupSpec(identifier: "voice", name: "Voice") {
        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Polyphony,
            identifier: "polyphony",
            name: "Polyphony",
            units: .indexed,
            valueRange: 1...32,
            defaultValue: 16,
            flags: [.flag_IsReadable] // Read-only since we pre-allocate voices
        )

        ParameterSpec(
            address: ModalAttractorsExtensionParameterAddress.kParam_Personality,
            identifier: "personality",
            name: "Personality",
            units: .indexed,
            valueRange: 0...1,
            defaultValue: 0,
            valueStrings: ["Resonator", "Self-Oscillator"]
        )
    }
}

// Extension to make parameter addresses work with ParameterSpec
extension ParameterSpec {
    init(
        address: ModalAttractorsExtensionParameterAddress,
        identifier: String,
        name: String,
        units: AudioUnitParameterUnit,
        valueRange: ClosedRange<AUValue>,
        defaultValue: AUValue,
        unitName: String? = nil,
        flags: AudioUnitParameterOptions = [.flag_IsWritable, .flag_IsReadable],
        valueStrings: [String]? = nil,
        dependentParameters: [NSNumber]? = nil
    ) {
        self.init(
            address: address.rawValue,
            identifier: identifier,
            name: name,
            units: units,
            valueRange: valueRange,
            defaultValue: defaultValue,
            unitName: unitName,
            flags: flags,
            valueStrings: valueStrings,
            dependentParameters: dependentParameters
        )
    }
}
