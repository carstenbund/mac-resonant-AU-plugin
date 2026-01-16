//
//  ModalAttractorsExtensionMainView.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI
import AudioToolbox
import os

private let log = Logger(subsystem: "com.bund.media.ModalAttractorsExtension", category: "MainView")

/// Root view for Modal Attractors AUv3 plugin
/// Single view controller with internal tab/page switching
public struct ModalAttractorsExtensionMainView: View {
    @EnvironmentObject var parameterTree: ParameterTree
    @State private var selectedTab: Tab = .main

    public init() {
        log.info("🎨 DEBUG: ModalAttractorsExtensionMainView.init() - Main view created")
        NSLog("🎨 AUv3 DEBUG: ModalAttractorsExtensionMainView initialized")
    }

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
            .onChange(of: selectedTab) { oldValue, newValue in
                log.info("🎨 DEBUG: Tab changed from \(oldValue.title) to \(newValue.title)")
                NSLog("🎨 AUv3 DEBUG: Tab switched to: %@", newValue.title)
            }

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
        .frame(minWidth: 520, minHeight: 400)
        .onAppear {
            log.info("🎨 DEBUG: MainView.onAppear() - Tabbed view is now visible")
            NSLog("🎨 AUv3 DEBUG: MainView appeared - Custom tabbed UI is RENDERING")
        }
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

// MARK: - Preview

#Preview {
    ModalAttractorsExtensionMainView()
        .environmentObject(ParameterTree.preview)
        .frame(width: 520, height: 400)
}
