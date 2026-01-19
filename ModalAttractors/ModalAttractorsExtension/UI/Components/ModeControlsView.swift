//
//  ModeControlsView.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI

/// Simple mode controls with sliders and wave shape picker
struct ModeControlsView: View {
    @ObservedObject var mode: ModeParameters
    @ObservedObject var waveShape: ParameterWrapper
    let modeLabel: String

    var body: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            Text(modeLabel)
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            // Wave shape picker (compact menu style)
            HStack {
                Text("Wave:")
                    .font(.caption)
                    .foregroundColor(.secondary)

                Picker("Wave Shape", selection: Binding(
                    get: { Int(waveShape.value) },
                    set: { waveShape.value = Float($0) }
                )) {
                    if let valueStrings = waveShape.valueStrings {
                        ForEach(0..<valueStrings.count, id: \.self) { index in
                            Text(valueStrings[index]).tag(index)
                        }
                    }
                }
                .pickerStyle(.menu)
                .labelsHidden()
            }

            ParameterSlider(
                param: mode.frequency,
                label: "Frequency",
                showUnit: false,
                formatString: "%.2f"
            )

            ParameterSlider(
                param: mode.damping,
                label: "Damping",
                showUnit: false,
                formatString: "%.2f"
            )

            ParameterSlider(
                param: mode.weight,
                label: "Weight",
                showUnit: false,
                formatString: "%.2f"
            )
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
    }
}

#Preview {
    ModeControlsView(
        mode: ParameterTree.preview.mode0,
        waveShape: ParameterTree.preview.waveShapeParameter(nodeIndex: 0, modeIndex: 0),
        modeLabel: "MODE 0"
    )
    .padding()
    .frame(width: 400)
}
