//
//  ModalAttractorsExtensionMainView.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI
import AudioToolbox

/// Main SwiftUI view for the Modal Attractors extension control panel
///
/// This view provides a comprehensive interface for controlling the Modal Attractors
/// synthesis engine, organized into three main sections:
/// - Network: Topology and coupling controls
/// - Triggers: Excitation parameters
/// - Output: Master gain control
public struct ModalAttractorsExtensionMainView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    public init() {}

    public var body: some View {
        ScrollView {
            VStack(spacing: UIConstants.Spacing.large) {
                // Header
                VStack(spacing: UIConstants.Spacing.tiny) {
                    Text("Modal Attractors")
                        .font(.title2)
                        .fontWeight(.bold)

                    Text("Network-Coupled Resonator Synthesis")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                .padding(.top, UIConstants.Spacing.medium)

                Divider()

                // Network section
                NetworkControlsView()

                Divider()

                // Triggers section
                TriggersControlsView()

                Divider()

                // Drive/Output section
                DriveControlsView()
            }
            .padding()
        }
        .frame(
            minWidth: UIConstants.Sizes.windowMinWidth,
            minHeight: UIConstants.Sizes.windowMinHeight
        )
    }
}

#Preview {
    ModalAttractorsExtensionMainView()
        .environmentObject(ParameterTree.preview)
        .frame(width: 500, height: 700)
}
