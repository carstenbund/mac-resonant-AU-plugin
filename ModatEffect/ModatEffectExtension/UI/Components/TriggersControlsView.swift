//
//  TriggersControlsView.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI

/// Excitation/trigger controls
/// Provides controls for poke strength and duration
struct TriggersControlsView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    var body: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            // Section header
            Text("TRIGGERS")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            // Poke strength slider
            ParameterSlider(
                param: parameterTree.excitation.pokeStrength,
                label: "Strength",
                showUnit: false,
                formatString: "%.2f"
            )

            // Poke duration slider
            ParameterSlider(
                param: parameterTree.excitation.pokeDuration,
                label: "Duration",
                showUnit: true,
                formatString: "%.1f"
            )

            // Optional: Manual trigger button
            // TODO: Implement manual poke trigger mechanism
            Button(action: {
                // Future: Implement manual trigger
                // Could send MIDI event or set a momentary parameter
            }) {
                Text("TRIGGER POKE")
                    .font(UIConstants.Fonts.buttonLabel)
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, UIConstants.Spacing.small)
            }
            .buttonStyle(.bordered)
            .disabled(true) // Disabled until trigger mechanism is implemented
            .help("Manual trigger not yet implemented")
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
    }
}

#Preview {
    TriggersControlsView()
        .environmentObject(ParameterTree.preview)
        .padding()
        .frame(width: 400)
}
