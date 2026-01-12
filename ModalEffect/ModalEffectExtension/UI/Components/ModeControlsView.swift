//
//  ModeControlsView.swift
//  ModalEffectExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI

/// Simple mode controls with sliders
struct ModeControlsView: View {
    @ObservedObject var mode: ModeParameters
    let modeLabel: String

    var body: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            Text(modeLabel)
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

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
        modeLabel: "MODE 0"
    )
    .padding()
    .frame(width: 400)
}
