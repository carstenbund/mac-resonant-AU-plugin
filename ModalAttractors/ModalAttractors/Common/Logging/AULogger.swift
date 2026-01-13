//
//  AULogger.swift
//  ModalAttractors
//
//  Created by Claude on 1/13/26.
//

import Foundation
import os

/// Global logging utility for the ModalAttractors AUv3 plugin
/// Provides category-based logging throughout the application and extension
public enum AULogger {

    // MARK: - Subsystems

    private enum Subsystem {
        static let app = "com.bund.media.ModalAttractors"
        static let audioUnit = "com.bund.media.ModalAttractorsExtension"
    }

    // MARK: - Categories

    /// Main app logging
    public static let app = Logger(subsystem: Subsystem.app, category: "App")

    /// Audio engine and playback
    public static let audio = Logger(subsystem: Subsystem.app, category: "Audio")

    /// Audio Unit host model
    public static let host = Logger(subsystem: Subsystem.app, category: "Host")

    /// Audio Unit extension logging
    public static let audioUnit = Logger(subsystem: Subsystem.audioUnit, category: "AudioUnit")

    /// Audio Unit view controller
    public static let viewController = Logger(subsystem: Subsystem.audioUnit, category: "ViewController")

    /// MIDI handling
    public static let midi = Logger(subsystem: Subsystem.app, category: "MIDI")

    /// UI and view updates
    public static let ui = Logger(subsystem: Subsystem.app, category: "UI")

    /// Custom category logger
    /// - Parameters:
    ///   - category: The logging category
    ///   - isExtension: Whether this is for the AUv3 extension (default: false)
    /// - Returns: A configured Logger instance
    public static func custom(category: String, isExtension: Bool = false) -> Logger {
        let subsystem = isExtension ? Subsystem.audioUnit : Subsystem.app
        return Logger(subsystem: subsystem, category: category)
    }
}
