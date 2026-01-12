//
//  ModalEffectExtensionMainView.swift
//  ModalEffectExtension
//
//  Simplified UI for ModalEffect audio effect
//

import SwiftUI
import AudioToolbox

/// Root view for ModalEffect AUv3 plugin
/// Simple single-page interface for the 5 effect parameters
public struct ModalEffectExtensionMainView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    public init() {}

    public var body: some View {
        VStack(spacing: 20) {
            // Header
            VStack(spacing: 4) {
                Text("ModalEffect")
                    .font(.title2)
                    .fontWeight(.bold)

                Text("Resonant Body Effect")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            .padding(.top, 12)

            Divider()

            // Effect Parameters
            ScrollView {
                VStack(spacing: 16) {
                    EffectParameterRow(
                        title: "Body Size",
                        subtitle: "Small = High Pitch, Large = Low Pitch",
                        parameter: parameterTree.effect.bodySize
                    )

                    EffectParameterRow(
                        title: "Material",
                        subtitle: "Soft = Short Decay, Hard = Long Ring",
                        parameter: parameterTree.effect.material
                    )

                    EffectParameterRow(
                        title: "Excite",
                        subtitle: "How Much Input Drives the Resonator",
                        parameter: parameterTree.effect.excite
                    )

                    EffectParameterRow(
                        title: "Morph",
                        subtitle: "Pitch Tracking: Fixed ← → Follows Input",
                        parameter: parameterTree.effect.morph
                    )

                    EffectParameterRow(
                        title: "Mix",
                        subtitle: "Dry/Wet Balance",
                        parameter: parameterTree.effect.mix
                    )
                }
                .padding(.horizontal, 16)
            }

            Spacer()
        }
        .frame(minWidth: 400, minHeight: 500)
        .background(Color(NSColor.windowBackgroundColor))
    }
}

// MARK: - Parameter Row Component

struct EffectParameterRow: View {
    let title: String
    let subtitle: String
    @ObservedObject var parameter: ParameterWrapper

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Title and value
            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text(title)
                        .font(.headline)
                    Text(subtitle)
                        .font(.caption)
                        .foregroundColor(.secondary)
                }

                Spacer()

                Text(String(format: "%.2f", parameter.value))
                    .font(.system(.body, design: .monospaced))
                    .foregroundColor(.secondary)
            }

            // Slider
            Slider(
                value: Binding(
                    get: { parameter.value },
                    set: { parameter.value = $0 }
                ),
                in: parameter.minValue...parameter.maxValue
            )
        }
        .padding(.vertical, 8)
        .padding(.horizontal, 12)
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(8)
    }
}

// MARK: - Preview

struct ModalEffectExtensionMainView_Previews: PreviewProvider {
    static var previews: some View {
        ModalEffectExtensionMainView()
            .environmentObject(ParameterTree.preview)
    }
}
