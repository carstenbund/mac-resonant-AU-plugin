//
//  ParameterTree.swift
//  ModalEffectExtension
//
//  Simplified parameter tree for ModalEffect audio effect
//

import Foundation
import AudioToolbox
import Combine
import SwiftUI

/// Main parameter tree wrapper for SwiftUI integration
/// This bridges the AU parameter tree to SwiftUI's observable system
public class ParameterTree: ObservableObject {
    public let effect: EffectParameters

    private let auParameterTree: AUParameterTree

    public init(auParameterTree: AUParameterTree) {
        self.auParameterTree = auParameterTree
        self.effect = EffectParameters(auParameterTree: auParameterTree)
    }

    /// Create a mock parameter tree for SwiftUI previews
    @MainActor
    public static var preview: ParameterTree {
        let tree = ModalEffectExtensionParameterSpecs.createAUParameterTree()
        return ParameterTree(auParameterTree: tree)
    }
}

/// Effect parameter group (5 parameters)
public class EffectParameters: ObservableObject {
    @Published public var bodySize: ParameterWrapper
    @Published public var material: ParameterWrapper
    @Published public var excite: ParameterWrapper
    @Published public var morph: ParameterWrapper
    @Published public var mix: ParameterWrapper

    init(auParameterTree: AUParameterTree) {
        // Fetch parameters from tree by address
        let bodySize = auParameterTree.parameter(withAddress: ModalEffectExtensionParameterAddress.param_BodySize.rawValue)!
        let material = auParameterTree.parameter(withAddress: ModalEffectExtensionParameterAddress.param_Material.rawValue)!
        let excite = auParameterTree.parameter(withAddress: ModalEffectExtensionParameterAddress.param_Excite.rawValue)!
        let morph = auParameterTree.parameter(withAddress: ModalEffectExtensionParameterAddress.param_Morph.rawValue)!
        let mix = auParameterTree.parameter(withAddress: ModalEffectExtensionParameterAddress.param_Mix.rawValue)!

        self.bodySize = ParameterWrapper(parameter: bodySize)
        self.material = ParameterWrapper(parameter: material)
        self.excite = ParameterWrapper(parameter: excite)
        self.morph = ParameterWrapper(parameter: morph)
        self.mix = ParameterWrapper(parameter: mix)
    }
}

/// Wraps a single AUParameter for SwiftUI @Binding compatibility
public class ParameterWrapper: ObservableObject {
    @Published public var value: AUValue {
        didSet {
            parameter.setValue(value, originator: nil)
        }
    }

    public let parameter: AUParameter

    init(parameter: AUParameter) {
        self.parameter = parameter
        self.value = parameter.value

        // Observe parameter changes from the AU (e.g., host automation)
        parameter.token(byAddingParameterObserver: { [weak self] _, _ in
            DispatchQueue.main.async {
                self?.value = parameter.value
            }
        })
    }

    public var displayName: String {
        parameter.displayName
    }

    public var minValue: AUValue {
        parameter.minValue
    }

    public var maxValue: AUValue {
        parameter.maxValue
    }

    public var unit: AudioUnitParameterUnit {
        parameter.unit
    }
}
