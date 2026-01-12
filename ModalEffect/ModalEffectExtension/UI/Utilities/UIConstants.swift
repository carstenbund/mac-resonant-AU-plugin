//
//  UIConstants.swift
//  ModalEffectExtension
//
//  Created by Carsten on 1/8/26.
//

import SwiftUI

/// UI constants for consistent styling across the Modal Attractors control panel
enum UIConstants {
    // MARK: - Colors

    enum Colors {
        static let primary = Color.blue
        static let secondary = Color.gray
        static let accent = Color.accentColor

        static let background = Color(NSColor.controlBackgroundColor)
        static let sectionBackground = Color.gray.opacity(0.1)

        static let textPrimary = Color.primary
        static let textSecondary = Color.secondary
    }

    // MARK: - Spacing

    enum Spacing {
        static let tiny: CGFloat = 4
        static let small: CGFloat = 8
        static let medium: CGFloat = 12
        static let large: CGFloat = 20
        static let extraLarge: CGFloat = 32
    }

    // MARK: - Sizes

    enum Sizes {
        static let knobSize: CGFloat = 60
        static let sliderHeight: CGFloat = 20
        static let minimumTouchTarget: CGFloat = 44

        static let windowMinWidth: CGFloat = 400
        static let windowMinHeight: CGFloat = 500
    }

    // MARK: - Corner Radius

    enum CornerRadius {
        static let small: CGFloat = 4
        static let medium: CGFloat = 8
        static let large: CGFloat = 12
    }

    // MARK: - Fonts

    enum Fonts {
        static let sectionTitle: Font = .headline
        static let parameterLabel: Font = .caption
        static let parameterValue: Font = .caption
        static let buttonLabel: Font = .body
    }
}
