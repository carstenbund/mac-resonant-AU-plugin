//
//  ModalEffectAUViewController.swift
//  ModalEffectExtension
//
//  Created by Carsten on 1/8/26.
//

import CoreAudioKit
import SwiftUI
import Combine
import os

#if os(macOS)
import AppKit
#endif

private let log = Logger(subsystem: "com.bund.media.ModalEffectExtension", category: "AUViewController")

/// Custom AUViewController subclass that hosts the SwiftUI view
/// This is the principal class for the extension - it conforms to AUAudioUnitFactory
/// to properly support both in-process and out-of-process instantiation.
///
/// CRITICAL for out-of-process AUv3:
/// - The view MUST be set up immediately in viewDidLoad (not lazily)
/// - The audio unit binding happens after view setup
/// - preferredContentSize must be overridden
/// - Class must be @objc for runtime discovery via NSExtensionPrincipalClass
public class ModalEffectAUViewController: AUViewController, AUAudioUnitFactory {

    // MARK: - Initialization

    /// Initialize without a nib - view is created programmatically in loadView()
    public override init(nibName nibNameOrNil: NSNib.Name?, bundle nibBundleOrNil: Bundle?) {
        super.init(nibName: nil, bundle: nil)
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
    }

    // MARK: - Audio Unit Factory

    /// The audio unit instance created by this factory
    var audioUnit: ModalEffectExtensionAudioUnit? {
        didSet {
            // When audio unit becomes available, bind parameters to UI
            if let au = audioUnit {
                bindAudioUnit(au)
            }
        }
    }

    /// Observation token for parameter value changes
    private var observation: NSKeyValueObservation?

    /// Creates the audio unit when requested by the host
    /// This is called by the system when the AU is instantiated
    @objc
    public func createAudioUnit(with componentDescription: AudioComponentDescription) throws -> AUAudioUnit {
        log.info("Creating audio unit...")

        let au = try ModalEffectExtensionAudioUnit(
            componentDescription: componentDescription,
            options: []
        )

        // Observe allParameterValues to ensure host can set initial values
        observation = au.observe(\.allParameterValues, options: [.new]) { [weak au] _, _ in
            guard let audioUnit = au, let tree = audioUnit.parameterTree else { return }
            // This ensures the Audio Unit gets initial values from the host
            for param in tree.allParameters {
                param.value = param.value
            }
        }

        log.info("Audio unit created successfully")

        // Set the audio unit - this triggers binding via didSet
        DispatchQueue.main.async { [weak self] in
            self?.audioUnit = au
        }

        return au
    }

    // MARK: - View Controller

    private var hostingController: NSHostingController<AnyView>?

    /// Observable wrapper for parameter tree that can be updated when AU becomes available
    private let parameterTreeHolder = ParameterTreeHolder()

    // MARK: - Content Size (Critical for out-of-process)

    /// Override preferredContentSize to ensure the host knows our size
    /// This is REQUIRED for out-of-process AUv3 to properly size the view
    public override var preferredContentSize: NSSize {
        get {
            return NSSize(
                width: UIConstants.Sizes.windowMinWidth,
                height: UIConstants.Sizes.windowMinHeight
            )
        }
        set {
            // Allow host to set size if needed
            super.preferredContentSize = newValue
        }
    }

    // MARK: - View Lifecycle

    /// Override loadView to create the view programmatically
    /// This is REQUIRED for NSViewController subclasses that don't use a nib
    public override func loadView() {
        log.info("loadView called - creating view programmatically")
        // Create a plain NSView as the root view
        // The hosting controller will be added as a subview in viewDidLoad
        self.view = NSView(frame: NSRect(x: 0, y: 0,
                                         width: UIConstants.Sizes.windowMinWidth,
                                         height: UIConstants.Sizes.windowMinHeight))
        self.view.wantsLayer = true
    }

    public override func viewDidLoad() {
        super.viewDidLoad()
        log.info("viewDidLoad called - setting up view immediately")

        // CRITICAL: Set up the view hierarchy IMMEDIATELY
        // Do NOT wait for the audio unit - the view must be ready when
        // requestViewController() is called by the host
        setupHostingController()
    }

    /// Bind the audio unit's parameter tree to the UI
    /// Called when audioUnit becomes available
    private func bindAudioUnit(_ au: ModalEffectExtensionAudioUnit) {
        guard let paramTree = au.parameterTree else {
            log.error("Audio unit has no parameter tree")
            return
        }

        log.info("Binding audio unit parameter tree to UI")

        // Create wrapper and update the holder (triggers UI update)
        let wrapper = ParameterTree(auParameterTree: paramTree)
        parameterTreeHolder.parameterTree = wrapper

        log.info("Parameter tree bound successfully")
    }

    /// Set up the SwiftUI hosting controller
    /// This creates the view hierarchy immediately - parameter binding happens later
    private func setupHostingController() {
        log.info("Setting up hosting controller with custom tabbed view")

        // Remove existing hosting controller if present
        if let existing = hostingController {
            existing.view.removeFromSuperview()
            existing.removeFromParent()
        }

        // Create the main view - it observes parameterTreeHolder for updates
        let rootView = AnyView(
            ModalEffectExtensionRootView()
                .environmentObject(parameterTreeHolder)
        )

        let hosting = NSHostingController(rootView: rootView)
        hosting.preferredContentSize = preferredContentSize

        // Add hosting controller as child view controller
        addChild(hosting)
        view.addSubview(hosting.view)

        // Set initial frame before adding constraints (helps with initial layout)
        hosting.view.frame = view.bounds

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

// MARK: - Parameter Tree Holder

/// Observable class that holds the parameter tree
/// This allows the view to be created before the audio unit is available,
/// and update when the parameter tree becomes available
class ParameterTreeHolder: ObservableObject {
    @Published var parameterTree: ParameterTree?
}

// MARK: - Root View with Loading State

/// Root view that handles the loading state while waiting for parameter tree
struct ModalEffectExtensionRootView: View {
    @EnvironmentObject var holder: ParameterTreeHolder

    var body: some View {
        Group {
            if let paramTree = holder.parameterTree {
                // Parameter tree is available - show main view
                ModalEffectExtensionMainView()
                    .environmentObject(paramTree)
            } else {
                // Still loading - show placeholder
                VStack(spacing: 16) {
                    ProgressView()
                        .scaleEffect(1.5)
                    Text("Initializing...")
                        .font(.headline)
                        .foregroundColor(.secondary)
                    Text("Modal Attractors")
                        .font(.caption)
                        .foregroundColor(.secondary.opacity(0.7))
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        .frame(
            minWidth: UIConstants.Sizes.windowMinWidth,
            minHeight: UIConstants.Sizes.windowMinHeight
        )
    }
}
