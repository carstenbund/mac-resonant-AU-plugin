//
//  ModalAttractorsAUViewController.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import CoreAudioKit
import SwiftUI
import os

private let log = Logger(subsystem: "com.bund.media.ModalAttractorsExtension", category: "AUViewController")

/// Custom AUViewController subclass that hosts the SwiftUI view
/// This is the principal class for the extension - it conforms to AUAudioUnitFactory
/// to properly support both in-process and out-of-process instantiation.
public class ModalAttractorsAUViewController: AUViewController, AUAudioUnitFactory {

    // MARK: - Audio Unit Factory

    /// The audio unit instance created by this factory
    var audioUnit: ModalAttractorsExtensionAudioUnit?

    /// Observation token for parameter value changes
    private var observation: NSKeyValueObservation?

    /// Creates the audio unit when requested by the host
    /// This is called by the system when the AU is instantiated
    @objc
    public func createAudioUnit(with componentDescription: AudioComponentDescription) throws -> AUAudioUnit {
        log.info("Creating audio unit...")

        let au = try ModalAttractorsExtensionAudioUnit(
            componentDescription: componentDescription,
            options: []
        )

        audioUnit = au

        // Observe allParameterValues to ensure host can set initial values
        observation = au.observe(\.allParameterValues, options: [.new]) { [weak au] _, _ in
            guard let audioUnit = au, let tree = audioUnit.parameterTree else { return }
            // This ensures the Audio Unit gets initial values from the host
            for param in tree.allParameters {
                param.value = param.value
            }
        }

        log.info("Audio unit created successfully")

        // Configure the view with the parameter tree if view is already loaded
        DispatchQueue.main.async { [weak self] in
            self?.configureViewIfNeeded()
        }

        return au
    }

    // MARK: - View Controller

    private var hostingController: NSHostingController<AnyView>?
    private var paramTreeWrapper: ParameterTree?
    private var isConfigured = false

    public override func viewDidLoad() {
        super.viewDidLoad()
        log.info("viewDidLoad called")

        // Try to configure immediately if audio unit is already available
        configureViewIfNeeded()
    }

    /// Configures the view with the parameter tree from the audio unit
    /// Safe to call multiple times - will only configure once
    private func configureViewIfNeeded() {
        guard !isConfigured else { return }
        guard isViewLoaded else { return }

        // Try to get parameter tree from our audio unit
        if let au = audioUnit, let paramTree = au.parameterTree {
            log.info("Configuring view with parameter tree")
            paramTreeWrapper = ParameterTree(auParameterTree: paramTree)
            setupHostingController()
            isConfigured = true
        } else {
            log.warning("Audio unit or parameter tree not available yet - view will be configured later")
            // Set up a placeholder or empty view that will be replaced
            setupPlaceholderView()
        }
    }

    private func setupPlaceholderView() {
        // Show a loading indicator while waiting for the audio unit
        let placeholderView = NSHostingController(rootView:
            AnyView(
                VStack {
                    ProgressView()
                    Text("Loading...")
                        .foregroundColor(.secondary)
                }
                .frame(minWidth: UIConstants.Sizes.windowMinWidth,
                       minHeight: UIConstants.Sizes.windowMinHeight)
            )
        )

        addChild(placeholderView)
        view.addSubview(placeholderView.view)

        placeholderView.view.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            placeholderView.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            placeholderView.view.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            placeholderView.view.topAnchor.constraint(equalTo: view.topAnchor),
            placeholderView.view.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])

        hostingController = placeholderView
    }

    private func setupHostingController() {
        guard let paramTree = paramTreeWrapper else {
            log.error("Cannot setup hosting controller without parameter tree")
            return
        }

        log.info("Setting up hosting controller with custom tabbed view")

        // Remove placeholder if it exists
        if let existing = hostingController {
            existing.view.removeFromSuperview()
            existing.removeFromParent()
        }

        // Create the main view with the parameter tree environment object
        let rootView = AnyView(
            ModalAttractorsExtensionMainView()
                .environmentObject(paramTree)
        )

        let hosting = NSHostingController(rootView: rootView)
        hosting.preferredContentSize = NSSize(
            width: UIConstants.Sizes.windowMinWidth,
            height: UIConstants.Sizes.windowMinHeight
        )

        // Add hosting controller as child view controller
        addChild(hosting)
        view.addSubview(hosting.view)

        // Set up auto-layout constraints to fill the parent view
        hosting.view.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            hosting.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            hosting.view.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            hosting.view.topAnchor.constraint(equalTo: view.topAnchor),
            hosting.view.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])

        hostingController = hosting
        log.info("Hosting controller setup complete")
    }
}
