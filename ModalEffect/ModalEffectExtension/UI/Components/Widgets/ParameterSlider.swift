//
//  ParameterSlider.swift
//  ModalEffectExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI

/// A reusable slider component for AU parameters
/// Displays a labeled slider with current value display
struct ParameterSlider: View {
    @ObservedObject var param: ParameterWrapper
    let label: String?
    let showUnit: Bool
    let formatString: String

    /// Initialize with a parameter wrapper
    /// - Parameters:
    ///   - param: The parameter wrapper to bind to
    ///   - label: Optional custom label (uses parameter name if nil)
    ///   - showUnit: Whether to show the unit in the value display
    ///   - formatString: Format string for value display (default "%.2f")
    init(
        param: ParameterWrapper,
        label: String? = nil,
        showUnit: Bool = true,
        formatString: String = "%.2f"
    ) {
        self.param = param
        self.label = label
        self.showUnit = showUnit
        self.formatString = formatString
    }

    var body: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.tiny) {
            HStack {
                Text(label ?? param.name)
                    .font(UIConstants.Fonts.parameterLabel)
                    .foregroundColor(UIConstants.Colors.textSecondary)

                Spacer()

                Text(formattedValue)
                    .font(UIConstants.Fonts.parameterValue)
                    .foregroundColor(UIConstants.Colors.textPrimary)
            }

            Slider(
                value: $param.value,
                in: param.minValue...param.maxValue
            )
            .tint(UIConstants.Colors.primary)
        }
        .accessibilityElement(children: .combine)
        .accessibilityLabel(label ?? param.name)
        .accessibilityValue(formattedValue)
    }

    private var formattedValue: String {
        let formatted: String

        // Check if this is an indexed parameter with value strings
        if param.unit == .indexed, let valueStrings = param.valueStrings {
            let index = Int(param.value)
            if index < valueStrings.count {
                return valueStrings[index]
            }
        }

        // Format based on unit
        switch param.unit {
        case .linearGain:
            formatted = String(format: formatString, param.value)
        case .milliseconds:
            formatted = String(format: formatString, param.value)
            return showUnit ? "\(formatted) ms" : formatted
        case .percent:
            let percent = param.value * 100
            formatted = String(format: "%.0f", percent)
            return "\(formatted)%"
        default:
            formatted = String(format: formatString, param.value)
        }

        return formatted
    }
}

#Preview {
    VStack(spacing: 20) {
        ParameterSlider(
            param: ParameterTree.preview.global.masterGain,
            label: "Master Gain"
        )

        ParameterSlider(
            param: ParameterTree.preview.global.couplingStrength,
            label: "Coupling"
        )

        ParameterSlider(
            param: ParameterTree.preview.excitation.pokeDuration,
            label: "Duration"
        )
    }
    .padding()
    .frame(width: 300)
}
