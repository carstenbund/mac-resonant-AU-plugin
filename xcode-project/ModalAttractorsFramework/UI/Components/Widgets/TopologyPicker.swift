//
//  TopologyPicker.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI

/// A picker for selecting network topology
/// Displays topology options from the parameter's value strings
struct TopologyPicker: View {
    @ObservedObject var param: ParameterWrapper

    var body: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.tiny) {
            Text("Topology")
                .font(UIConstants.Fonts.parameterLabel)
                .foregroundColor(UIConstants.Colors.textSecondary)

            if let valueStrings = param.valueStrings {
                Picker("Topology", selection: Binding(
                    get: { Int(param.value) },
                    set: { param.value = Float($0) }
                )) {
                    ForEach(0..<valueStrings.count, id: \.self) { index in
                        Text(valueStrings[index])
                            .tag(index)
                    }
                }
                .pickerStyle(.menu)
                .labelsHidden()
            } else {
                Text("No topology options available")
                    .font(UIConstants.Fonts.parameterLabel)
                    .foregroundColor(UIConstants.Colors.textSecondary)
            }
        }
        .accessibilityElement(children: .combine)
        .accessibilityLabel("Topology")
        .accessibilityValue(currentTopologyName)
    }

    private var currentTopologyName: String {
        if let valueStrings = param.valueStrings {
            let index = Int(param.value)
            if index < valueStrings.count {
                return valueStrings[index]
            }
        }
        return "Unknown"
    }
}

#Preview {
    VStack(spacing: 20) {
        TopologyPicker(param: ParameterTree.preview.global.topology)
    }
    .padding()
    .frame(width: 300)
}
