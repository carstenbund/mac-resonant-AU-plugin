//
//  VoiceControlsView.swift
//  ModalEffectExtension
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

            // Personality selector (indexed parameter)
            VStack(alignment: .leading, spacing: UIConstants.Spacing.tiny) {
                Text("Personality")
                    .font(UIConstants.Fonts.parameterLabel)
                    .foregroundColor(UIConstants.Colors.textSecondary)

                if let valueStrings = parameterTree.voice.personality.valueStrings {
                    Picker("Personality", selection: Binding(
                        get: { Int(parameterTree.voice.personality.value) },
                        set: { parameterTree.voice.personality.value = Float($0) }
                    )) {
                        ForEach(0..<valueStrings.count, id: \.self) { index in
                            Text(valueStrings[index]).tag(index)
                        }
                    }
                    .pickerStyle(.menu)
                    .labelsHidden()
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
}

#Preview {
    VoiceControlsView()
        .environmentObject(ParameterTree.preview)
        .padding()
        .frame(width: 400)
}
