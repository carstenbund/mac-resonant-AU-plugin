//
//  CharacterEditorView.swift
//  ModalAttractorsExtension
//
//  Character editor page for power users
//  Allows editing mode parameters, excitation, and personality for custom character creation
//

import SwiftUI

/// Character editor page for advanced users
/// Provides detailed control over all character parameters:
/// - Mode frequencies, damping, and weights (4 modes)
/// - Excitation parameters (poke strength and duration)
/// - Personality (resonator vs self-oscillator)
struct CharacterEditorView: View {
    @EnvironmentObject var parameterTree: ParameterTree
    @StateObject private var presetManager = CharacterPresetManager.shared
    @Environment(\.dismiss) var dismiss

    @State private var selectedNodeIndex: Int = 0
    @State private var selectedTemplateIndex: Int = 0
    @State private var selectedCustomPresetID: UUID?
    @State private var showingSaveDialog: Bool = false
    @State private var showingPresetList: Bool = false
    @State private var presetName: String = ""

    // Character template names (matching NodeCharacter.cpp)
    private let characterTemplates = [
        "Vibrant Bass",
        "Dark Node",
        "Bright Bell",
        "Glassy Shimmer",
        "Drone Hub"
    ]

    var body: some View {
        VStack(spacing: 0) {
            // Custom header bar (macOS doesn't have navigation bar)
            HStack {
                Button("Back") {
                    dismiss()
                }
                .keyboardShortcut(.cancelAction)

                Spacer()

                Text("Character Editor")
                    .font(.headline)

                Spacer()

                // Placeholder for symmetry
                Button("") {}.hidden()
            }
            .padding()
            .background(Color(nsColor: .controlBackgroundColor))

            Divider()

            ScrollView {
                VStack(spacing: UIConstants.Spacing.large) {
                    // Header
                    headerSection

                    Divider()

                    // Template loader
                    templateSection

                    Divider()

                    // Mode parameters (4 modes)
                    modeParametersSection

                    Divider()

                    // Excitation parameters
                    excitationSection

                    Divider()

                    // Personality
                    personalitySection

                    Divider()

                    // Action buttons
                    actionsSection
                }
                .padding()
            }
        }
        .frame(minWidth: 600, minHeight: 700)
    }

    // MARK: - Header Section

    private var headerSection: some View {
        VStack(spacing: UIConstants.Spacing.small) {
            Text("Character Editor")
                .font(.title2)
                .fontWeight(.bold)

            Text("Edit character parameters for advanced control")
                .font(.caption)
                .foregroundColor(.secondary)

            // Node selector
            HStack {
                Text("Editing Node:")
                    .font(UIConstants.Fonts.sectionTitle)
                    .foregroundColor(UIConstants.Colors.textSecondary)

                Picker("Node", selection: $selectedNodeIndex) {
                    ForEach(0..<5) { index in
                        Text("Node \(index)").tag(index)
                    }
                }
                .pickerStyle(.menu)
                .labelsHidden()
            }
        }
    }

    // MARK: - Template Section

    private var templateSection: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            Text("LOAD TEMPLATE")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            // Built-in templates
            VStack(alignment: .leading, spacing: UIConstants.Spacing.small) {
                Text("Built-in Templates")
                    .font(.caption)
                    .foregroundColor(.secondary)

                Picker("Template", selection: $selectedTemplateIndex) {
                    ForEach(0..<characterTemplates.count, id: \.self) { index in
                        Text(characterTemplates[index]).tag(index)
                    }
                }
                .pickerStyle(.menu)

                Button("Load Template") {
                    loadTemplate()
                }
                .buttonStyle(.bordered)
            }

            // Custom presets
            if !presetManager.presets.isEmpty {
                Divider()

                VStack(alignment: .leading, spacing: UIConstants.Spacing.small) {
                    Text("Custom Presets (\(presetManager.presets.count))")
                        .font(.caption)
                        .foregroundColor(.secondary)

                    Button("Browse Custom Presets") {
                        showingPresetList = true
                    }
                    .buttonStyle(.bordered)
                }
            }

            Text("Note: Loading a template overwrites current editor values")
                .font(.caption2)
                .foregroundColor(.secondary)
                .italic()
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
        .sheet(isPresented: $showingPresetList) {
            PresetBrowserView(presetManager: presetManager) { preset in
                loadCustomPreset(preset)
                showingPresetList = false
            }
        }
    }

    // MARK: - Mode Parameters Section

    private var modeParametersSection: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.large) {
            Text("MODE PARAMETERS")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            Text("4 internal modal oscillators per node")
                .font(.caption)
                .foregroundColor(.secondary)

            // Mode 0
            ModeControlsView(mode: parameterTree.mode0, modeLabel: "MODE 0")

            // Mode 1
            ModeControlsView(mode: parameterTree.mode1, modeLabel: "MODE 1")

            // Mode 2
            ModeControlsView(mode: parameterTree.mode2, modeLabel: "MODE 2")

            // Mode 3
            ModeControlsView(mode: parameterTree.mode3, modeLabel: "MODE 3")
        }
    }

    // MARK: - Excitation Section

    private var excitationSection: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            Text("EXCITATION")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            ParameterSlider(
                param: parameterTree.excitation.pokeStrength,
                label: "Poke Strength",
                showUnit: false,
                formatString: "%.2f"
            )

            ParameterSlider(
                param: parameterTree.excitation.pokeDuration,
                label: "Poke Duration",
                showUnit: true,
                formatString: "%.1f"
            )
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
    }

    // MARK: - Personality Section

    private var personalitySection: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            Text("PERSONALITY")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            Text("Behavioral characteristics of the node")
                .font(.caption)
                .foregroundColor(.secondary)

            Picker("Personality", selection: Binding(
                get: { Int(parameterTree.voice.personality.value) },
                set: { parameterTree.voice.personality.value = Float($0) }
            )) {
                if let valueStrings = parameterTree.voice.personality.valueStrings {
                    ForEach(0..<valueStrings.count, id: \.self) { index in
                        Text(valueStrings[index]).tag(index)
                    }
                }
            }
            .pickerStyle(.segmented)
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
    }

    // MARK: - Actions Section

    private var actionsSection: some View {
        VStack(spacing: UIConstants.Spacing.medium) {
            // Apply to node button
            Button {
                applyToNode()
            } label: {
                Label("Apply to Node \(selectedNodeIndex)", systemImage: "checkmark.circle.fill")
                    .font(UIConstants.Fonts.buttonLabel)
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, UIConstants.Spacing.small)
            }
            .buttonStyle(.borderedProminent)
            .help("Apply current editor parameters to the selected node")

            // Save preset button
            Button {
                showingSaveDialog = true
            } label: {
                Label("Save as Custom Preset", systemImage: "square.and.arrow.down")
                    .font(UIConstants.Fonts.buttonLabel)
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, UIConstants.Spacing.small)
            }
            .buttonStyle(.bordered)
            .help("Save current parameters as a custom character preset")

            if !presetManager.presets.isEmpty {
                Text("You have \(presetManager.presets.count) custom preset(s) saved")
                    .font(.caption2)
                    .foregroundColor(.secondary)
            }
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
        .alert("Save Custom Preset", isPresented: $showingSaveDialog) {
            TextField("Preset Name", text: $presetName)
            Button("Save") {
                savePreset()
            }
            Button("Cancel", role: .cancel) {
                presetName = ""
            }
        } message: {
            Text("Enter a name for this custom character preset. It will be saved permanently and available in the template browser.")
        }
    }

    // MARK: - Actions

    private func loadTemplate() {
        // Load template character values into editor parameters
        // Template values are defined in NodeCharacter.cpp
        let templates: [[(Float, Float, Float)]] = [
            // Vibrant Bass (low, rich fundamentals)
            [(1.0, 0.8, 1.0), (2.01, 1.0, 0.7), (3.02, 1.2, 0.5), (4.05, 1.5, 0.3)],
            // Dark Node (low damping, complex)
            [(1.0, 0.5, 1.0), (1.9, 0.6, 0.8), (2.8, 0.7, 0.6), (3.5, 0.9, 0.4)],
            // Bright Bell (harmonic, ringing)
            [(1.0, 1.2, 0.8), (2.0, 1.4, 1.0), (3.0, 1.6, 0.7), (4.0, 2.0, 0.5)],
            // Glassy Shimmer (high partials)
            [(2.0, 0.8, 0.6), (3.5, 1.0, 0.8), (5.2, 1.2, 1.0), (7.1, 1.5, 0.7)],
            // Drone Hub (sustained, coupled)
            [(1.0, 0.3, 1.0), (1.5, 0.4, 0.9), (2.2, 0.5, 0.8), (3.1, 0.6, 0.7)]
        ]

        guard selectedTemplateIndex < templates.count else { return }
        let template = templates[selectedTemplateIndex]

        // Apply to mode parameters
        parameterTree.mode0.frequency.value = template[0].0
        parameterTree.mode0.damping.value = template[0].1
        parameterTree.mode0.weight.value = template[0].2

        parameterTree.mode1.frequency.value = template[1].0
        parameterTree.mode1.damping.value = template[1].1
        parameterTree.mode1.weight.value = template[1].2

        parameterTree.mode2.frequency.value = template[2].0
        parameterTree.mode2.damping.value = template[2].1
        parameterTree.mode2.weight.value = template[2].2

        parameterTree.mode3.frequency.value = template[3].0
        parameterTree.mode3.damping.value = template[3].1
        parameterTree.mode3.weight.value = template[3].2

        // Set template-specific excitation and personality
        // (Future: could be part of template data)
        switch selectedTemplateIndex {
        case 0: // Vibrant Bass
            parameterTree.excitation.pokeStrength.value = 0.7
            parameterTree.excitation.pokeDuration.value = 15.0
        case 1: // Dark Node
            parameterTree.excitation.pokeStrength.value = 0.5
            parameterTree.excitation.pokeDuration.value = 20.0
        case 2: // Bright Bell
            parameterTree.excitation.pokeStrength.value = 0.8
            parameterTree.excitation.pokeDuration.value = 10.0
        case 3: // Glassy Shimmer
            parameterTree.excitation.pokeStrength.value = 0.6
            parameterTree.excitation.pokeDuration.value = 12.0
        case 4: // Drone Hub
            parameterTree.excitation.pokeStrength.value = 0.4
            parameterTree.excitation.pokeDuration.value = 25.0
        default:
            break
        }
    }

    private func applyToNode() {
        // Apply current editor parameters to selected node
        // This sets the node's character to match the editor values
        // Note: In current implementation, characters are preset-based
        // This would require custom character support in DSP

        // For now, just set the node to the selected template index
        // (Future enhancement: create custom characters in DSP)
        switch selectedNodeIndex {
        case 0:
            parameterTree.nodeCharacters.node0.value = Float(selectedTemplateIndex)
        case 1:
            parameterTree.nodeCharacters.node1.value = Float(selectedTemplateIndex)
        case 2:
            parameterTree.nodeCharacters.node2.value = Float(selectedTemplateIndex)
        case 3:
            parameterTree.nodeCharacters.node3.value = Float(selectedTemplateIndex)
        case 4:
            parameterTree.nodeCharacters.node4.value = Float(selectedTemplateIndex)
        default:
            break
        }
    }

    private func loadCustomPreset(_ preset: CharacterPreset) {
        // Apply the custom preset to the editor parameters
        preset.apply(to: parameterTree)
    }

    private func savePreset() {
        // Save current editor parameters as custom preset
        guard !presetName.isEmpty else { return }

        let preset = CharacterPreset(name: presetName, from: parameterTree)
        presetManager.savePreset(preset)

        // Clear the name
        presetName = ""
    }
}

// MARK: - Preset Browser View

/// Simple preset browser/picker view
struct PresetBrowserView: View {
    @ObservedObject var presetManager: CharacterPresetManager
    @Environment(\.dismiss) var dismiss

    let onSelect: (CharacterPreset) -> Void

    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Button("Done") {
                    dismiss()
                }
                .keyboardShortcut(.cancelAction)

                Spacer()

                Text("Custom Presets")
                    .font(.headline)

                Spacer()

                // Placeholder for symmetry
                Button("") {}.hidden()
            }
            .padding()
            .background(Color(nsColor: .controlBackgroundColor))

            Divider()

            // Preset list
            if presetManager.presets.isEmpty {
                VStack(spacing: UIConstants.Spacing.medium) {
                    Spacer()
                    Text("No Custom Presets")
                        .font(.title3)
                        .foregroundColor(.secondary)
                    Text("Create presets in the Character Editor")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    Spacer()
                }
            } else {
                List {
                    ForEach(presetManager.presets) { preset in
                        Button {
                            onSelect(preset)
                        } label: {
                            HStack {
                                VStack(alignment: .leading, spacing: 4) {
                                    Text(preset.name)
                                        .font(.headline)

                                    Text(preset.dateCreated, style: .date)
                                        .font(.caption)
                                        .foregroundColor(.secondary)
                                }

                                Spacer()
                            }
                            .contentShape(Rectangle())
                        }
                        .buttonStyle(.plain)
                    }
                    .onDelete { indexSet in
                        indexSet.forEach { index in
                            presetManager.deletePreset(at: index)
                        }
                    }
                }
            }
        }
        .frame(minWidth: 400, minHeight: 300)
    }
}

#Preview {
    CharacterEditorView()
        .environmentObject(ParameterTree.preview)
}
