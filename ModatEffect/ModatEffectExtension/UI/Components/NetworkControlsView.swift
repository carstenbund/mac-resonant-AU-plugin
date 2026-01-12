//
//  NetworkControlsView.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI

/// Network configuration controls
/// Provides controls for topology selection and coupling strength
struct NetworkControlsView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    var body: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            // Section header
            Text("NETWORK")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            // Topology picker
            TopologyPicker(param: parameterTree.global.topology)

            // Coupling strength slider
            ParameterSlider(
                param: parameterTree.global.couplingStrength,
                label: "Coupling Strength",
                showUnit: false,
                formatString: "%.2f"
            )

            // Node count slider
            ParameterSlider(
                param: parameterTree.global.nodeCount,
                label: "Node Count",
                showUnit: false,
                formatString: "%.0f"
            )
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
    }
}

#Preview {
    NetworkControlsView()
        .environmentObject(ParameterTree.preview)
        .padding()
        .frame(width: 400)
}
