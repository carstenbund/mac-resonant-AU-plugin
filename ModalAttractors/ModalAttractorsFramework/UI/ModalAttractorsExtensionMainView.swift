//
//  ModalAttractorsExtensionMainView.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI
import AudioToolbox

/// Root view for Modal Attractors AUv3 plugin
/// Single view controller with internal tab/page switching
public struct ModalAttractorsExtensionMainView: View {
    @EnvironmentObject var parameterTree: ParameterTree
    @State private var selectedTab: Tab = .main

    public init() {}

    // MARK: - Tab Definition

    enum Tab: Int, CaseIterable, Identifiable {
        case main
        case characterEditor

        var id: Int { rawValue }

        var title: String {
            switch self {
            case .main: return "Main"
            case .characterEditor: return "Character Editor"
            }
        }
    }

    // MARK: - Body

    public var body: some View {
        VStack(spacing: 12) {
            // Segmented tab picker
            Picker("", selection: $selectedTab) {
                ForEach(Tab.allCases) { tab in
                    Text(tab.title).tag(tab)
                }
            }
            .pickerStyle(.segmented)

            // Tab content
            Group {
                switch selectedTab {
                case .main:
                    MainTabView()
                case .characterEditor:
                    CharacterEditorTabView()
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .padding(12)
        .frame(minWidth: 900, minHeight: 650)
    }
}

// MARK: - Main Tab

/// Main control page - simple interface for most users
struct MainTabView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    var body: some View {
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
                .padding(.top, UIConstants.Spacing.small)

                Divider()

                // Output section (top)
                DriveControlsView()

                Divider()

                // Network and Node Characters (side-by-side)
                HStack(alignment: .top, spacing: UIConstants.Spacing.large) {
                    NetworkControlsView()
                        .frame(maxWidth: .infinity)

                    NodeCharactersView()
                        .frame(maxWidth: .infinity)
                }

                Divider()

                // Routing & Behavior (bottom)
                RoutingControlsView()
            }
            .padding()
        }
        .background(
            GeometryReader { geo in
                Image("main-tab-background")
                    .resizable()
                    .scaledToFill()
                    .frame(width: geo.size.width, height: geo.size.height)
                    .clipped()
            }
            .allowsHitTesting(false)
        )
        .clipped()
    }
}

// MARK: - Preview

#Preview {
    ModalAttractorsExtensionMainView()
        .environmentObject(ParameterTree.preview)
        .frame(width: 900, height: 650)
}
