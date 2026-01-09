//
//  NodeCharactersView.swift
//  ModalAttractorsExtension
//
//  Node Character System controls
//

import SwiftUI

/// Node Character controls
/// Provides character selection for each of the 5 fixed nodes
struct NodeCharactersView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    var body: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            // Section header
            Text("NODE CHARACTERS")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            Text("Each node has distinct sonic character")
                .font(.caption2)
                .foregroundColor(UIConstants.Colors.textSecondary)

            VStack(spacing: UIConstants.Spacing.small) {
                // Node 0
                CharacterPicker(
                    param: parameterTree.nodeCharacters.node0,
                    label: "Node 0",
                    color: .blue
                )

                // Node 1
                CharacterPicker(
                    param: parameterTree.nodeCharacters.node1,
                    label: "Node 1",
                    color: .purple
                )

                // Node 2
                CharacterPicker(
                    param: parameterTree.nodeCharacters.node2,
                    label: "Node 2",
                    color: .green
                )

                // Node 3
                CharacterPicker(
                    param: parameterTree.nodeCharacters.node3,
                    label: "Node 3",
                    color: .orange
                )

                // Node 4
                CharacterPicker(
                    param: parameterTree.nodeCharacters.node4,
                    label: "Node 4",
                    color: .red
                )
            }
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
    }
}

/// Character picker widget for a single node
struct CharacterPicker: View {
    @ObservedObject var param: ParameterWrapper
    let label: String
    let color: Color

    var body: some View {
        HStack {
            // Node label with color indicator
            HStack(spacing: 4) {
                Circle()
                    .fill(color)
                    .frame(width: 8, height: 8)

                Text(label)
                    .font(.caption)
                    .fontWeight(.medium)
                    .frame(width: 60, alignment: .leading)
            }

            // Character picker
            if let valueStrings = param.valueStrings {
                Picker("", selection: Binding(
                    get: { Int(param.value) },
                    set: { param.value = Float($0) }
                )) {
                    ForEach(0..<valueStrings.count, id: \.self) { index in
                        Text(valueStrings[index])
                            .tag(index)
                    }
                }
                .pickerStyle(.menu)
                .frame(maxWidth: .infinity)
            }
        }
    }
}

#Preview {
    NodeCharactersView()
        .environmentObject(ParameterTree.preview)
        .padding()
        .frame(width: 400)
}
