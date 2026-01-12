//
//  DriveControlsView.swift
//  ModalEffectExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI

/// Output gain controls
/// Provides master gain control via rotary knob
struct DriveControlsView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    var body: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            // Section header
            Text("OUTPUT")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            HStack(spacing: UIConstants.Spacing.extraLarge) {
                // Master gain knob
                ParameterKnob(
                    param: parameterTree.global.masterGain,
                    label: "Gain"
                )

                Spacer()

                // Future: Drive parameter can be added here
                // when implemented in Phase 3 of the roadmap
                /*
                VStack {
                    ParameterSlider(
                        param: parameterTree.global.drive,
                        label: "Drive",
                        showUnit: false
                    )
                }
                */
            }
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
    }
}

#Preview {
    DriveControlsView()
        .environmentObject(ParameterTree.preview)
        .padding()
        .frame(width: 400)
}
