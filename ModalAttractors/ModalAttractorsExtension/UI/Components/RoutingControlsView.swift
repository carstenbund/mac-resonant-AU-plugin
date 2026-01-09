//
//  RoutingControlsView.swift
//  ModalAttractorsExtension
//
//  Routing and behavior controls
//

import SwiftUI

/// Routing & behavior controls
/// Provides controls for note routing mode and multi-excitation behavior
struct RoutingControlsView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    var body: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            // Section header
            Text("ROUTING & BEHAVIOR")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            // Note Routing picker
            HStack {
                Text("Note Routing")
                    .font(.subheadline)
                    .fontWeight(.medium)
                    .frame(width: 120, alignment: .leading)

                if let valueStrings = parameterTree.routing.noteRouting.valueStrings {
                    Picker("", selection: Binding(
                        get: { Int(parameterTree.routing.noteRouting.value) },
                        set: { parameterTree.routing.noteRouting.value = Float($0) }
                    )) {
                        ForEach(0..<valueStrings.count, id: \.self) { index in
                            Text(valueStrings[index])
                                .tag(index)
                        }
                    }
                    .pickerStyle(.segmented)
                }
            }

            // Help text for routing
            Text(routingHelpText)
                .font(.caption2)
                .foregroundColor(UIConstants.Colors.textSecondary)
                .fixedSize(horizontal: false, vertical: true)

            Divider()
                .padding(.vertical, UIConstants.Spacing.tiny)

            // Multi-Excitation picker
            HStack {
                Text("Multi-Excitation")
                    .font(.subheadline)
                    .fontWeight(.medium)
                    .frame(width: 120, alignment: .leading)

                if let valueStrings = parameterTree.routing.multiExcite.valueStrings {
                    Picker("", selection: Binding(
                        get: { Int(parameterTree.routing.multiExcite.value) },
                        set: { parameterTree.routing.multiExcite.value = Float($0) }
                    )) {
                        ForEach(0..<valueStrings.count, id: \.self) { index in
                            Text(valueStrings[index])
                                .tag(index)
                        }
                    }
                    .pickerStyle(.segmented)
                }
            }

            // Help text for multi-excite
            Text(multiExciteHelpText)
                .font(.caption2)
                .foregroundColor(UIConstants.Colors.textSecondary)
                .fixedSize(horizontal: false, vertical: true)
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
    }

    private var routingHelpText: String {
        let mode = Int(parameterTree.routing.noteRouting.value)
        switch mode {
        case 0: // Round-Robin
            return "Notes distributed evenly across all 5 nodes in sequence."
        case 1: // Pitch Zones
            return "Low notes → Node 0 (bass), high notes → Node 4 (treble)."
        default:
            return ""
        }
    }

    private var multiExciteHelpText: String {
        let mode = Int(parameterTree.routing.multiExcite.value)
        switch mode {
        case 0: // Re-Trigger
            return "New notes reset node state (percussive, clear attack)."
        case 1: // Accumulate
            return "New notes add to ringing nodes (rich, layered interaction)."
        default:
            return ""
        }
    }
}

#Preview {
    RoutingControlsView()
        .environmentObject(ParameterTree.preview)
        .padding()
        .frame(width: 400)
}
