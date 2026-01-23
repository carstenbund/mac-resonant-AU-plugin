//
//  VoiceControlsView.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI

/// Voice configuration controls
struct VoiceControlsView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    var body: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            Text("VOICE")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            // Personality slider (continuous parameter)
            VStack(alignment: .leading, spacing: UIConstants.Spacing.tiny) {
                HStack {
                    Text("Personality")
                        .font(UIConstants.Fonts.parameterLabel)
                        .foregroundColor(UIConstants.Colors.textSecondary)

                    Spacer()

                    Text(personalityLabel(parameterTree.voice.personality.value))
                        .font(UIConstants.Fonts.parameterValue)
                        .foregroundColor(UIConstants.Colors.accent)
                }

                Slider(value: Binding(
                    get: { parameterTree.voice.personality.value },
                    set: { parameterTree.voice.personality.value = $0 }
                ), in: 0.0...1.0)

                // Range labels
                HStack {
                    Text("Resonator")
                        .font(.caption2)
                        .foregroundColor(UIConstants.Colors.textSecondary)
                    Spacer()
                    Text("Sustained")
                        .font(.caption2)
                        .foregroundColor(UIConstants.Colors.textSecondary)
                    Spacer()
                    Text("Self-Osc")
                        .font(.caption2)
                        .foregroundColor(UIConstants.Colors.textSecondary)
                }
            }

            // Polyphony display (read-only)
            HStack {
                Text("Polyphony")
                    .font(UIConstants.Fonts.parameterLabel)
                    .foregroundColor(UIConstants.Colors.textSecondary)

                Spacer()

                Text(String(format: "%.0f", parameterTree.voice.polyphony.value))
                    .font(UIConstants.Fonts.parameterValue)
                    .foregroundColor(UIConstants.Colors.textSecondary)
            }
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
    }

    // MARK: - Helper Functions

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

#Preview {
    VoiceControlsView()
        .environmentObject(ParameterTree.preview)
        .padding()
        .frame(width: 400)
}
