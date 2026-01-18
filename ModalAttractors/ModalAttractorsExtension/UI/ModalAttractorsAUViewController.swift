//
//  ModalAttractorsAUViewController.swift
//  ModalAttractorsExtension
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

private let log = Logger(subsystem: "com.bund.media.ModalAttractorsExtension", category: "AUViewController")

// MARK: - FourCharCode Extension for Debugging

extension UInt32 {
    /// Convert FourCharCode to readable string (e.g., 'aumi' from 1635085673)
    var fourCharString: String {
        let bytes: [UInt8] = [
            UInt8((self >> 24) & 0xFF),
            UInt8((self >> 16) & 0xFF),
            UInt8((self >> 8) & 0xFF),
            UInt8(self & 0xFF)
        ]
        return String(bytes: bytes, encoding: .utf8) ?? "????"
    }
}

/// Custom AUViewController subclass that hosts the SwiftUI view
/// This is the principal class for the extension - it conforms to AUAudioUnitFactory
/// to properly support both in-process and out-of-process instantiation.
///
/// CRITICAL for out-of-process AUv3:
/// - The view MUST be set up immediately in viewDidLoad (not lazily)
/// - The audio unit binding happens after view setup
/// - preferredContentSize must be overridden
/// - Class must be @objc for runtime discovery via NSExtensionPrincipalClass
@objc(ModalAttractorsAUViewController)
public class ModalAttractorsAUViewController: AUViewController, AUAudioUnitFactory {

    // MARK: - Initialization

    /// Initialize without a nib - view is created programmatically in loadView()
    public override init(nibName nibNameOrNil: NSNib.Name?, bundle nibBundleOrNil: Bundle?) {
        super.init(nibName: nil, bundle: nil)
        log.info("🔵 DEBUG: ModalAttractorsAUViewController.init() called - View Controller is being instantiated by host")
        NSLog("🔵 AUv3 DEBUG: ModalAttractorsAUViewController.init() - ViewController INSTANTIATED")
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        log.info("🔵 DEBUG: ModalAttractorsAUViewController.init(coder:) called")
        NSLog("🔵 AUv3 DEBUG: ModalAttractorsAUViewController.init(coder:) - ViewController instantiated from coder")
    }

    // MARK: - Audio Unit Factory

    /// The audio unit instance created by this factory
    var audioUnit: ModalAttractorsExtensionAudioUnit? {
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
        log.info("🟢 DEBUG: createAudioUnit(with:) called - Factory method invoked")
        NSLog("🟢 AUv3 DEBUG: createAudioUnit called - type=\(componentDescription.componentType.fourCharString), subtype=\(componentDescription.componentSubType.fourCharString), manufacturer=\(componentDescription.componentManufacturer.fourCharString)")

        let au = try ModalAttractorsExtensionAudioUnit(
            componentDescription: componentDescription,
            options: []
        )

        log.info("🟢 DEBUG: Audio unit instantiated successfully")
        NSLog("🟢 AUv3 DEBUG: Audio unit created, type: %@", String(describing: type(of: au)))

        // Observe allParameterValues to ensure host can set initial values
        observation = au.observe(\.allParameterValues, options: [.new]) { [weak au] _, _ in
            guard let audioUnit = au, let tree = audioUnit.parameterTree else { return }
            // This ensures the Audio Unit gets initial values from the host
            for param in tree.allParameters {
                param.value = param.value
            }
        }

        log.info("🟢 DEBUG: Parameter observation set up")

        // Set the audio unit - this triggers binding via didSet
        DispatchQueue.main.async { [weak self] in
            NSLog("🟢 AUv3 DEBUG: Setting audioUnit on main queue (async)")
            self?.audioUnit = au
        }

        log.info("🟢 DEBUG: Returning audio unit to host")
        NSLog("🟢 AUv3 DEBUG: createAudioUnit returning AU instance to host")
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
        log.info("🟡 DEBUG: loadView() called - creating root NSView")
        NSLog("🟡 AUv3 DEBUG: loadView - Creating root view programmatically (size: %fx%f)", UIConstants.Sizes.windowMinWidth, UIConstants.Sizes.windowMinHeight)

        // Create a plain NSView as the root view
        // The hosting controller will be added as a subview in viewDidLoad
        self.view = NSView(frame: NSRect(x: 0, y: 0,
                                         width: UIConstants.Sizes.windowMinWidth,
                                         height: UIConstants.Sizes.windowMinHeight))
        self.view.wantsLayer = true

        // DEBUG: Set bright background to verify view is visible
        #if DEBUG
        self.view.layer?.backgroundColor = NSColor.systemBlue.withAlphaComponent(0.1).cgColor
        log.info("🟡 DEBUG: Root view background set to light blue for debugging")
        #endif

        log.info("🟡 DEBUG: loadView complete - root view created")
        NSLog("🟡 AUv3 DEBUG: loadView complete")
    }

    public override func viewDidLoad() {
        super.viewDidLoad()
        log.info("🟠 DEBUG: viewDidLoad() called - setting up view hierarchy")
        NSLog("🟠 AUv3 DEBUG: viewDidLoad - Setting up SwiftUI hosting controller")

        // CRITICAL: Set up the view hierarchy IMMEDIATELY
        // Do NOT wait for the audio unit - the view must be ready when
        // requestViewController() is called by the host
        setupHostingController()

        log.info("🟠 DEBUG: viewDidLoad complete")
        NSLog("🟠 AUv3 DEBUG: viewDidLoad complete - view hierarchy ready")
    }

    /// Bind the audio unit's parameter tree to the UI
    /// Called when audioUnit becomes available
    private func bindAudioUnit(_ au: ModalAttractorsExtensionAudioUnit) {
        log.info("🔶 DEBUG: bindAudioUnit() called")
        NSLog("🔶 AUv3 DEBUG: bindAudioUnit - Attempting to bind parameter tree to UI")

        guard let paramTree = au.parameterTree else {
            log.error("🔴 DEBUG ERROR: Audio unit has no parameter tree!")
            NSLog("🔴 AUv3 DEBUG ERROR: Audio unit has no parameter tree")
            return
        }

        log.info("🔶 DEBUG: Parameter tree found with \(paramTree.allParameters.count) parameters")
        NSLog("🔶 AUv3 DEBUG: Binding parameter tree with %d parameters", paramTree.allParameters.count)

        // Create wrapper and update the holder (triggers UI update)
        let wrapper = ParameterTree(auParameterTree: paramTree)
        parameterTreeHolder.parameterTree = wrapper

        log.info("🔶 DEBUG: Parameter tree bound successfully - UI should update")
        NSLog("🔶 AUv3 DEBUG: Parameter tree bound to UI successfully")
    }

    /// Set up the SwiftUI hosting controller
    /// This creates the view hierarchy immediately - parameter binding happens later
    private func setupHostingController() {
        log.info("🟣 DEBUG: setupHostingController() - Creating SwiftUI hosting controller")
        NSLog("🟣 AUv3 DEBUG: setupHostingController - Building SwiftUI view hierarchy")

        // Remove existing hosting controller if present
        if let existing = hostingController {
            log.info("🟣 DEBUG: Removing existing hosting controller")
            existing.view.removeFromSuperview()
            existing.removeFromParent()
        }

        // Create the main view - it observes parameterTreeHolder for updates
        let rootView = AnyView(
            ModalAttractorsExtensionRootView()
                .environmentObject(parameterTreeHolder)
        )

        log.info("🟣 DEBUG: Creating NSHostingController with rootView")
        let hosting = NSHostingController(rootView: rootView)
        hosting.preferredContentSize = preferredContentSize

        log.info("🟣 DEBUG: Hosting controller created, preferredContentSize=\(NSStringFromSize(preferredContentSize))")
        NSLog("🟣 AUv3 DEBUG: Hosting controller preferredContentSize: %fx%f", preferredContentSize.width, preferredContentSize.height)

        // Add hosting controller as child view controller
        addChild(hosting)
        view.addSubview(hosting.view)

        log.info("🟣 DEBUG: Hosting controller added as child, view added as subview")

        // Set initial frame before adding constraints (helps with initial layout)
        hosting.view.frame = view.bounds

        log.info("🟣 DEBUG: Initial frame set to view.bounds: \(NSStringFromRect(view.bounds))")

        // Set up auto-layout constraints to fill the parent view
        hosting.view.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            hosting.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            hosting.view.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            hosting.view.topAnchor.constraint(equalTo: view.topAnchor),
            hosting.view.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])

        log.info("🟣 DEBUG: Auto-layout constraints activated")

        hostingController = hosting
        log.info("🟣 DEBUG: Hosting controller setup complete - SwiftUI view should be visible")
        NSLog("🟣 AUv3 DEBUG: setupHostingController complete - custom view ready")
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
struct ModalAttractorsExtensionRootView: View {
    @EnvironmentObject var holder: ParameterTreeHolder

    var body: some View {
        Group {
            if let paramTree = holder.parameterTree {
                // Parameter tree is available - show main view
                ModalAttractorsExtensionMainView()
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
