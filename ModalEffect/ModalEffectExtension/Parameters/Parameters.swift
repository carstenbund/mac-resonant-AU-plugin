//
//  Parameters.swift
//  ModalEffectExtension
//
//  Resonant Body Effect Parameter Specification
//

import Foundation
import AudioToolbox

// Parameter tree specification for Resonant Body Effect
let ModalEffectExtensionParameterSpecs = ParameterTreeSpec {

    // MARK: - Effect Parameters
    ParameterGroupSpec(identifier: "effect", name: "Effect") {
        ParameterSpec(
            address: ModalEffectExtensionParameterAddress.param_BodySize,
            identifier: "bodySize",
            name: "Body Size",
            units: .generic,
            valueRange: 0.0...1.0,
            defaultValue: 0.5
        )

        ParameterSpec(
            address: ModalEffectExtensionParameterAddress.param_Material,
            identifier: "material",
            name: "Material",
            units: .generic,
            valueRange: 0.0...1.0,
            defaultValue: 0.5
        )

        ParameterSpec(
            address: ModalEffectExtensionParameterAddress.param_Excite,
            identifier: "excite",
            name: "Excite",
            units: .generic,
            valueRange: 0.0...1.0,
            defaultValue: 0.5
        )

        ParameterSpec(
            address: ModalEffectExtensionParameterAddress.param_Morph,
            identifier: "morph",
            name: "Morph",
            units: .generic,
            valueRange: 0.0...1.0,
            defaultValue: 0.0
        )

        ParameterSpec(
            address: ModalEffectExtensionParameterAddress.param_Mix,
            identifier: "mix",
            name: "Mix",
            units: .generic,
            valueRange: 0.0...1.0,
            defaultValue: 0.5
        )
    }
}

// Extension to make parameter addresses work with ParameterSpec
extension ParameterSpec {
    init(
        address: ModalEffectExtensionParameterAddress,
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
