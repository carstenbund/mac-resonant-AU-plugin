//
//  CharacterEditorTabView.swift
//  ModalAttractorsExtension
//
//  Character editor tab for the main view
//  Embedded version without navigation/dismiss - used as a tab
//

import SwiftUI

/// Character editor as a tab (no navigation wrapper)
///
/// NOTE: This editor uses a "global editor + assign" workflow:
/// - Mode0-3, excitation, and personality parameters are GLOBAL (shared across all editing)
/// - "selectedNodeIndex" determines which node receives the assignment when you click "Apply to Node"
/// - The editor controls always show the current global parameter values
/// - To edit a different character: load a template/preset, adjust parameters, then apply to a node
///
/// Now using ParameterStore for clean separation of data and presentation
struct CharacterEditorTabView: View {
    @EnvironmentObject var parameterTree: ParameterTree
    @StateObject private var presetManager = CharacterPresetManager.shared
    @StateObject private var parameterStore = ParameterStore()

    @State private var selectedNodeIndex: Int = 0
    @State private var selectedTemplateIndex: Int = 0
    @State private var selectedCustomPresetID: UUID?
    @State private var showingSaveDialog: Bool = false
    @State private var showingPresetList: Bool = false
    @State private var presetName: String = ""

    var body: some View {
        ScrollView {
            VStack(spacing: UIConstants.Spacing.large) {
                // Header
                VStack(spacing: UIConstants.Spacing.small) {
                    Text("Character Editor")
                        .font(.title2)
                        .fontWeight(.bold)

                    Text("Edit character parameters for advanced control")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }

                Divider()

                // Node selector and Template loader (side-by-side)
                HStack(alignment: .top, spacing: UIConstants.Spacing.large) {
                    // Node selector
                    nodeSelectorSection
                        .frame(maxWidth: .infinity)

                    // Template loader
                    templateSection
                        .frame(maxWidth: .infinity)
                }

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
            .background(
                Image("editor-tab-background")
                    .resizable()
                    .scaledToFill()
                    .allowsHitTesting(false)
            )
            .clipped()
        }
    }

    // MARK: - Node Selector Section

    private var nodeSelectorSection: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            Text("SELECT NODE")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            VStack(alignment: .leading, spacing: UIConstants.Spacing.small) {
                Text("Editing wave shapes for:")
                    .font(.caption)
                    .foregroundColor(.secondary)

                Picker("Node", selection: $selectedNodeIndex) {
                    ForEach(0..<5) { index in
                        Text("Node \(index)").tag(index)
                    }
                }
                .pickerStyle(.menu)
                .onChange(of: selectedNodeIndex) { newValue in
                    // Update the DSP engine's editing node index for real-time parameter application
                    parameterTree.characterEditorState.editingNodeIndex.value = Float(newValue)
                }
            }

            Text("Note: Wave shapes are per-node, other parameters are shared")
                .font(.caption2)
                .foregroundColor(.secondary)
                .italic()
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
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
                    ForEach(0..<CharacterTemplates.names.count, id: \.self) { index in
                        Text(CharacterTemplates.names[index]).tag(index)
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

            Text("Editing wave shapes for Node \(selectedNodeIndex)")
                .font(.caption2)
                .foregroundColor(.secondary)
                .italic()

            // Row 1: Mode 0 and Mode 1 (side-by-side)
            HStack(alignment: .top, spacing: UIConstants.Spacing.large) {
                ModeControlsView(
                    mode: parameterTree.mode0,
                    waveShape: parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0),
                    modeLabel: "MODE 0"
                )
                .frame(maxWidth: .infinity)

                ModeControlsView(
                    mode: parameterTree.mode1,
                    waveShape: parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1),
                    modeLabel: "MODE 1"
                )
                .frame(maxWidth: .infinity)
            }

            // Row 2: Mode 2 and Mode 3 (side-by-side)
            HStack(alignment: .top, spacing: UIConstants.Spacing.large) {
                ModeControlsView(
                    mode: parameterTree.mode2,
                    waveShape: parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2),
                    modeLabel: "MODE 2"
                )
                .frame(maxWidth: .infinity)

                ModeControlsView(
                    mode: parameterTree.mode3,
                    waveShape: parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3),
                    modeLabel: "MODE 3"
                )
                .frame(maxWidth: .infinity)
            }
        }
    }

    // MARK: - Excitation Section

    private var excitationSection: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            Text("EXCITATION")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            // Rotary knobs for excitation parameters
            HStack(spacing: UIConstants.Spacing.large) {
                ParameterKnob(
                    param: parameterTree.excitation.pokeStrength,
                    label: "Strength",
                    size: 80,
                    formatString: "%.2f"
                )

                ParameterKnob(
                    param: parameterTree.excitation.pokeDuration,
                    label: "Duration",
                    size: 80,
                    formatString: "%.1f ms"
                )
            }
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

            VStack(alignment: .leading, spacing: UIConstants.Spacing.small) {
                HStack {
                    Text("Oscillation Mode")
                        .font(UIConstants.Fonts.parameterLabel)
                        .foregroundColor(UIConstants.Colors.textSecondary)

                    Spacer()

                    Text(personalityLabel(parameterTree.voice.personality.value))
                        .font(UIConstants.Fonts.parameterValue)
                        .foregroundColor(UIConstants.Colors.accent)
                }

                Slider(value: $parameterTree.voice.personality.value,
                       in: 0.0...1.0)

                // Detailed range descriptions
                VStack(alignment: .leading, spacing: 2) {
                    HStack {
                        Text("0.0 - 0.2")
                            .font(.caption2)
                            .foregroundColor(UIConstants.Colors.textSecondary)
                            .frame(width: 60, alignment: .leading)
                        Text("Pure resonator (natural decay)")
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                    HStack {
                        Text("0.2 - 0.5")
                            .font(.caption2)
                            .foregroundColor(UIConstants.Colors.textSecondary)
                            .frame(width: 60, alignment: .leading)
                        Text("Sustained resonance (self-drive)")
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                    HStack {
                        Text("0.5 - 1.0")
                            .font(.caption2)
                            .foregroundColor(UIConstants.Colors.textSecondary)
                            .frame(width: 60, alignment: .leading)
                        Text("Self-oscillator (Van der Pol)")
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                }
                .padding(.top, 4)
            }
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
        // Load template from CharacterTemplates (clean separation of data from UI)
        guard let template = CharacterTemplates.template(at: selectedTemplateIndex) else {
            return
        }

        // Apply template to parameter store, then apply store to parameter tree
        template.apply(to: parameterStore, nodeIndex: selectedNodeIndex)
        parameterStore.applyTo(parameterTree, nodeIndex: selectedNodeIndex)
    }

    private func applyToNode() {
        // Apply current editor parameters to selected node
        // For now, just set the node to the selected template index
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
        // Apply the custom preset to the parameter store, then to parameter tree
        preset.apply(to: parameterStore, nodeIndex: selectedNodeIndex)
        parameterStore.applyTo(parameterTree, nodeIndex: selectedNodeIndex)
    }

    private func savePreset() {
        // Save current editor parameters as custom preset (including wave shapes from current node)
        guard !presetName.isEmpty else { return }

        // Load current values into parameter store, then create preset
        parameterStore.loadFrom(parameterTree)
        let preset = CharacterPreset(name: presetName, from: parameterStore)
        presetManager.savePreset(preset)

        // Clear the name
        presetName = ""
    }

    /// Generate descriptive label for personality value
    private func personalityLabel(_ value: Float) -> String {
        switch value {
        case 0.0..<0.2:
            return String(format: "%.2f (Decay)", value)
        case 0.2..<0.5:
            return String(format: "%.2f (Sustain)", value)
        case 0.5..<0.8:
            return String(format: "%.2f (Active)", value)
        default:
            return String(format: "%.2f (Self-Osc)", value)
        }
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
                        .contextMenu {
                            Button("Delete", role: .destructive) {
                                presetManager.deletePreset(id: preset.id)
                            }
                        }
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
    CharacterEditorTabView()
        .environmentObject(ParameterTree.preview)
}
