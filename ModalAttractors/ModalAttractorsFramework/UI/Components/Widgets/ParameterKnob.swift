//
//  ParameterKnob.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI

/// A rotary knob control for AU parameters
/// Provides a circular knob interface with drag interaction
/// Supports Shift+drag for 10x fine adjustment precision
struct ParameterKnob: View {
    @ObservedObject var param: ParameterWrapper
    let label: String?
    let size: CGFloat
    let formatString: String?

    @State private var isDragging = false
    @State private var dragStart: CGPoint = .zero
    @State private var valueStart: Float = 0.0
    @State private var isShiftPressed = false

    /// Initialize with a parameter wrapper
    /// - Parameters:
    ///   - param: The parameter wrapper to bind to
    ///   - label: Optional custom label (uses parameter name if nil)
    ///   - size: Diameter of the knob
    ///   - formatString: Optional custom format string (e.g., "%.2f")
    init(
        param: ParameterWrapper,
        label: String? = nil,
        size: CGFloat = UIConstants.Sizes.knobSize,
        formatString: String? = nil
    ) {
        self.param = param
        self.label = label
        self.size = size
        self.formatString = formatString
    }

    var body: some View {
        VStack(spacing: UIConstants.Spacing.small) {
            // Rotary knob visual
            ZStack {
                // Background circle
                Circle()
                    .fill(UIConstants.Colors.sectionBackground)
                    .frame(width: size, height: size)

                // Value arc
                Circle()
                    .trim(from: 0, to: normalizedValue)
                    .stroke(UIConstants.Colors.primary, lineWidth: 4)
                    .frame(width: size, height: size)
                    .rotationEffect(.degrees(-90))

                // Pointer indicator
                Rectangle()
                    .fill(Color.primary)
                    .frame(width: 2, height: size * 0.3)
                    .offset(y: -size * 0.25)
                    .rotationEffect(.degrees(normalizedValue * 270 - 135))
            }
            .gesture(
                DragGesture()
                    .onChanged { gesture in
                        if !isDragging {
                            isDragging = true
                            dragStart = gesture.location
                            valueStart = param.value
                        }

                        // Check for Shift modifier (10x fine precision)
                        #if os(macOS)
                        isShiftPressed = NSEvent.modifierFlags.contains(.shift)
                        #else
                        isShiftPressed = false
                        #endif

                        // Vertical drag to change value
                        let delta = Float(gesture.location.y - dragStart.y)
                        let baseSensitivity: Float = (param.maxValue - param.minValue) / 200.0 // 200 pixels for full range
                        let sensitivity = isShiftPressed ? baseSensitivity / 10.0 : baseSensitivity // 10x finer with Shift
                        let newValue = valueStart - (delta * sensitivity)
                        param.value = min(max(newValue, param.minValue), param.maxValue)
                    }
                    .onEnded { _ in
                        isDragging = false
                        isShiftPressed = false
                    }
            )

            // Label
            Text(label ?? param.name)
                .font(UIConstants.Fonts.parameterLabel)
                .foregroundColor(UIConstants.Colors.textSecondary)

            // Value display with precision indicator
            HStack(spacing: 4) {
                Text(valueString)
                    .font(UIConstants.Fonts.parameterValue)
                    .foregroundColor(UIConstants.Colors.textPrimary)
                    .monospacedDigit()

                if isDragging && isShiftPressed {
                    Text("×0.1")
                        .font(.caption2)
                        .foregroundColor(.secondary)
                }
            }
        }
        .accessibilityElement(children: .combine)
        .accessibilityLabel(label ?? param.name)
        .accessibilityValue(valueString)
        .accessibilityAdjustableAction { direction in
            let step = (param.maxValue - param.minValue) / 20.0
            switch direction {
            case .increment:
                param.value = min(param.value + step, param.maxValue)
            case .decrement:
                param.value = max(param.value - step, param.minValue)
            @unknown default:
                break
            }
        }
    }

    /// Normalized value (0.0 to 1.0) for visual display
    private var normalizedValue: Double {
        Double((param.value - param.minValue) / (param.maxValue - param.minValue))
    }

    /// Formatted value string
    private var valueString: String {
        // Use custom format string if provided
        if let formatString = formatString {
            return String(format: formatString, param.value)
        }

        // Otherwise use unit-based formatting
        switch param.unit {
        case .linearGain:
            let percent = param.value * 100
            return String(format: "%.0f%%", percent)
        case .percent:
            let percent = param.value * 100
            return String(format: "%.0f%%", percent)
        case .milliseconds:
            return String(format: "%.1f ms", param.value)
        default:
            return String(format: "%.2f", param.value)
        }
    }
}

#Preview {
    HStack(spacing: 40) {
        ParameterKnob(
            param: ParameterTree.preview.global.masterGain,
            label: "Gain"
        )

        ParameterKnob(
            param: ParameterTree.preview.global.couplingStrength,
            label: "Coupling"
        )
    }
    .padding()
}
