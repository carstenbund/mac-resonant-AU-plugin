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
/// synthesis engine, organized into tabs:
/// - Main: Network topology, node characters, routing
/// - Editor: Advanced character parameter editing
public struct ModalAttractorsExtensionMainView: View {
    @EnvironmentObject var parameterTree: ParameterTree
    @State private var selectedTab = 0

    public init() {}

    public var body: some View {
        VStack(spacing: 0) {
            // Tab picker (segmented control)
            Picker("", selection: $selectedTab) {
                Text("Main").tag(0)
                Text("Character Editor").tag(1)
            }
            .pickerStyle(.segmented)
            .padding([.horizontal, .top])

            Divider()
                .padding(.top, 8)

            // Tab content
            Group {
                switch selectedTab {
                case 0:
                    mainTabContent
                case 1:
                    CharacterEditorTabView()
                        .environmentObject(parameterTree)
                default:
                    mainTabContent
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .frame(
            minWidth: UIConstants.Sizes.windowMinWidth,
            minHeight: UIConstants.Sizes.windowMinHeight
        )
    }

    // MARK: - Main Tab

    private var mainTabContent: some View {
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

                // Node Character System
                NodeCharactersView()

                Divider()

                // Routing & Behavior
                RoutingControlsView()

                Divider()

                // Drive/Output section
                DriveControlsView()
            }
            .padding()
        }
    }
}

#Preview {
    ModalAttractorsExtensionMainView()
        .environmentObject(ParameterTree.preview)
        .frame(width: 500, height: 700)
}
