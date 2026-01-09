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
    @State private var showingCharacterEditor = false

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

                // Node Character System
                NodeCharactersView()

                Divider()

                // Routing & Behavior
                RoutingControlsView()

                // Character Editor Button
                Button {
                    showingCharacterEditor = true
                } label: {
                    Label("Edit Characters...", systemImage: "slider.horizontal.3")
                        .font(UIConstants.Fonts.buttonLabel)
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, UIConstants.Spacing.small)
                }
                .buttonStyle(.bordered)
                .help("Open advanced character editor")

                Divider()

                // Mode 0-3 sections (for Character Editor - hide in main view for now)
                // TODO: Move these to a separate Character Editor page
                /*
                ModeControlsView(mode: parameterTree.mode0, modeLabel: "MODE 0")
                ModeControlsView(mode: parameterTree.mode1, modeLabel: "MODE 1")
                ModeControlsView(mode: parameterTree.mode2, modeLabel: "MODE 2")
                ModeControlsView(mode: parameterTree.mode3, modeLabel: "MODE 3")

                Divider()

                // Triggers section
                TriggersControlsView()

                Divider()

                // Voice section
                VoiceControlsView()

                Divider()
                */

                // Drive/Output section
                DriveControlsView()
            }
            .padding()
        }
        .frame(
            minWidth: UIConstants.Sizes.windowMinWidth,
            minHeight: UIConstants.Sizes.windowMinHeight
        )
        .sheet(isPresented: $showingCharacterEditor) {
            CharacterEditorView()
                .environmentObject(parameterTree)
        }
    }
}

#Preview {
    ModalAttractorsExtensionMainView()
        .environmentObject(ParameterTree.preview)
        .frame(width: 500, height: 700)
}
