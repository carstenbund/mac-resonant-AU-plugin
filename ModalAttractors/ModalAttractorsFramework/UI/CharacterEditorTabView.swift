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
/// Future enhancement: Per-node parameter editing would require node-specific parameter storage
struct CharacterEditorTabView: View {
    @EnvironmentObject var parameterTree: ParameterTree
    @StateObject private var presetManager = CharacterPresetManager.shared

    @State private var selectedNodeIndex: Int = 0
    @State private var selectedTemplateIndex: Int = 0
    @State private var selectedCustomPresetID: UUID?
    @State private var showingSaveDialog: Bool = false
    @State private var showingPresetList: Bool = false
    @State private var presetName: String = ""

    // Character template names (matching NodeCharacter.cpp + additional variations)
    private let characterTemplates = [
        "Vibrant Bass",
        "Dark Node",
        "Bright Bell",
        "Glassy Shimmer",
        "Drone Hub",
        "Metallic Strike",
        "Warm Pad",
        "Percussive Hit",
        "Resonant Bell",
        "Deep Rumble",
        "Harmonic Stack",
        "Detuned Chorus",
        "Mallet Tone",
        "Wind Chime",
        "Gong Wash"
    ]

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

            Text("Editing wave shapes for Node \(selectedNodeIndex)")
                .font(.caption2)
                .foregroundColor(.secondary)
                .italic()

            // Mode 0
            ModeControlsView(
                mode: parameterTree.mode0,
                waveShape: parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0),
                modeLabel: "MODE 0"
            )

            // Mode 1
            ModeControlsView(
                mode: parameterTree.mode1,
                waveShape: parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1),
                modeLabel: "MODE 1"
            )

            // Mode 2
            ModeControlsView(
                mode: parameterTree.mode2,
                waveShape: parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2),
                modeLabel: "MODE 2"
            )

            // Mode 3
            ModeControlsView(
                mode: parameterTree.mode3,
                waveShape: parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3),
                modeLabel: "MODE 3"
            )
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
        // Format: [(freq_multiplier, damping, weight)] for each of 4 modes
        let templates: [[(Float, Float, Float)]] = [
            // 0: Vibrant Bass (low, rich fundamentals)
            [(1.0, 0.8, 1.0), (2.01, 1.0, 0.7), (3.02, 1.2, 0.5), (4.05, 1.5, 0.3)],

            // 1: Dark Node (low damping, complex)
            [(1.0, 0.5, 1.0), (1.9, 0.6, 0.8), (2.8, 0.7, 0.6), (3.5, 0.9, 0.4)],

            // 2: Bright Bell (harmonic, ringing)
            [(1.0, 1.2, 0.8), (2.0, 1.4, 1.0), (3.0, 1.6, 0.7), (4.0, 2.0, 0.5)],

            // 3: Glassy Shimmer (high partials)
            [(2.0, 0.8, 0.6), (3.5, 1.0, 0.8), (5.2, 1.2, 1.0), (7.1, 1.5, 0.7)],

            // 4: Drone Hub (sustained, coupled)
            [(1.0, 0.3, 1.0), (1.5, 0.4, 0.9), (2.2, 0.5, 0.8), (3.1, 0.6, 0.7)],

            // 5: Metallic Strike (bright, sharp attack, fast decay)
            [(1.0, 2.0, 0.6), (3.14, 2.5, 0.8), (5.87, 3.0, 1.0), (8.23, 3.5, 0.7)],

            // 6: Warm Pad (smooth, sustained, low harmonics)
            [(1.0, 0.2, 1.0), (2.0, 0.25, 0.85), (3.0, 0.3, 0.7), (4.0, 0.4, 0.5)],

            // 7: Percussive Hit (fast decay, punchy)
            [(1.0, 3.0, 1.0), (2.5, 3.5, 0.6), (4.2, 4.0, 0.4), (6.7, 4.5, 0.2)],

            // 8: Resonant Bell (harmonic stack, long sustain)
            [(1.0, 0.6, 1.0), (2.0, 0.7, 0.9), (3.0, 0.8, 0.8), (4.0, 1.0, 0.7)],

            // 9: Deep Rumble (sub-bass focus, low partials)
            [(0.5, 0.5, 1.0), (1.0, 0.6, 0.9), (1.5, 0.8, 0.6), (2.0, 1.0, 0.4)],

            // 10: Harmonic Stack (perfect harmonic series)
            [(1.0, 1.0, 1.0), (2.0, 1.0, 0.8), (3.0, 1.0, 0.6), (4.0, 1.0, 0.4)],

            // 11: Detuned Chorus (slightly detuned, thick)
            [(1.0, 0.7, 1.0), (1.99, 0.7, 0.85), (2.98, 0.8, 0.7), (4.03, 0.9, 0.5)],

            // 12: Mallet Tone (wood/mallet character)
            [(1.0, 1.5, 1.0), (2.76, 1.8, 0.7), (4.18, 2.2, 0.5), (5.94, 2.5, 0.3)],

            // 13: Wind Chime (delicate, high partials)
            [(3.0, 0.9, 0.7), (4.5, 1.0, 0.8), (6.2, 1.1, 1.0), (8.7, 1.3, 0.8)],

            // 14: Gong Wash (complex inharmonic, evolving)
            [(1.0, 0.4, 0.8), (2.37, 0.5, 1.0), (3.86, 0.6, 0.9), (5.19, 0.7, 0.7)]
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

        // Set template-specific excitation, personality, and wave shapes
        // Wave shape indices: 0=Sine, 1=Sawtooth, 2=Triangle, 3=Square, 4=Pulse25%, 5=Pulse10%
        switch selectedTemplateIndex {
        case 0: // Vibrant Bass - rich fundamental with harmonics
            parameterTree.excitation.pokeStrength.value = 0.7
            parameterTree.excitation.pokeDuration.value = 15.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 1 // Sawtooth
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 2 // Triangle
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 0 // Sine
        case 1: // Dark Node - complex with sawtooth
            parameterTree.excitation.pokeStrength.value = 0.5
            parameterTree.excitation.pokeDuration.value = 20.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 1 // Sawtooth
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 3 // Square
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 1 // Sawtooth
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 2 // Triangle
        case 2: // Bright Bell - pure sine waves
            parameterTree.excitation.pokeStrength.value = 0.8
            parameterTree.excitation.pokeDuration.value = 10.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 0 // Sine
        case 3: // Glassy Shimmer - triangle for smoothness
            parameterTree.excitation.pokeStrength.value = 0.6
            parameterTree.excitation.pokeDuration.value = 12.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 2 // Triangle
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 2 // Triangle
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 2 // Triangle
        case 4: // Drone Hub - sustained with square
            parameterTree.excitation.pokeStrength.value = 0.4
            parameterTree.excitation.pokeDuration.value = 25.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 3 // Square
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 3 // Square
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 1 // Sawtooth
        case 5: // Metallic Strike - bright harmonics
            parameterTree.excitation.pokeStrength.value = 0.9
            parameterTree.excitation.pokeDuration.value = 5.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 3 // Square
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 1 // Sawtooth
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 3 // Square
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 4 // Pulse25%
        case 6: // Warm Pad - smooth sine and triangle
            parameterTree.excitation.pokeStrength.value = 0.3
            parameterTree.excitation.pokeDuration.value = 30.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 2 // Triangle
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 2 // Triangle
        case 7: // Percussive Hit - sharp attack with pulse
            parameterTree.excitation.pokeStrength.value = 1.0
            parameterTree.excitation.pokeDuration.value = 3.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 5 // Pulse10%
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 3 // Square
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 4 // Pulse25%
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 1 // Sawtooth
        case 8: // Resonant Bell - pure harmonics
            parameterTree.excitation.pokeStrength.value = 0.75
            parameterTree.excitation.pokeDuration.value = 12.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 2 // Triangle
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 0 // Sine
        case 9: // Deep Rumble - low with sawtooth
            parameterTree.excitation.pokeStrength.value = 0.6
            parameterTree.excitation.pokeDuration.value = 20.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 1 // Sawtooth
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 1 // Sawtooth
        case 10: // Harmonic Stack - perfect harmonics with sine
            parameterTree.excitation.pokeStrength.value = 0.65
            parameterTree.excitation.pokeDuration.value = 15.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 0 // Sine
        case 11: // Detuned Chorus - thick with sawtooth
            parameterTree.excitation.pokeStrength.value = 0.5
            parameterTree.excitation.pokeDuration.value = 18.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 1 // Sawtooth
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 1 // Sawtooth
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 2 // Triangle
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 1 // Sawtooth
        case 12: // Mallet Tone - wood character with mixed waves
            parameterTree.excitation.pokeStrength.value = 0.85
            parameterTree.excitation.pokeDuration.value = 8.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 2 // Triangle
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 3 // Square
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 1 // Sawtooth
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 2 // Triangle
        case 13: // Wind Chime - delicate with triangle
            parameterTree.excitation.pokeStrength.value = 0.4
            parameterTree.excitation.pokeDuration.value = 14.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 2 // Triangle
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 2 // Triangle
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 0 // Sine
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 2 // Triangle
        case 14: // Gong Wash - complex inharmonic with mixed waves
            parameterTree.excitation.pokeStrength.value = 0.7
            parameterTree.excitation.pokeDuration.value = 35.0
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 0).value = 1 // Sawtooth
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 1).value = 3 // Square
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 2).value = 1 // Sawtooth
            parameterTree.waveShapeParameter(nodeIndex: selectedNodeIndex, modeIndex: 3).value = 4 // Pulse25%
        default:
            break
        }
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
        // Apply the custom preset to the editor parameters (including wave shapes for current node)
        preset.apply(to: parameterTree, nodeIndex: selectedNodeIndex)
    }

    private func savePreset() {
        // Save current editor parameters as custom preset (including wave shapes from current node)
        guard !presetName.isEmpty else { return }

        let preset = CharacterPreset(name: presetName, from: parameterTree, nodeIndex: selectedNodeIndex)
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
