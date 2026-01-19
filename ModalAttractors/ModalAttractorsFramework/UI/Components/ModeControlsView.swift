//
//  ModeControlsView.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI

/// Mode controls with rotary knobs and wave shape picker
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

            // Rotary knobs for mode parameters
            HStack(spacing: UIConstants.Spacing.large) {
                ParameterKnob(
                    param: mode.frequency,
                    label: "Frequency",
                    size: 60,
                    formatString: "%.2f×"
                )

                ParameterKnob(
                    param: mode.damping,
                    label: "Damping",
                    size: 60,
                    formatString: "%.2f"
                )

                ParameterKnob(
                    param: mode.weight,
                    label: "Weight",
                    size: 60,
                    formatString: "%.2f"
                )
            }
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
