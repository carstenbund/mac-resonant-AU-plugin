//
//  ModalAttractorsAUViewController.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import CoreAudioKit
import SwiftUI
import AppKit

/// Custom AUViewController subclass that hosts the SwiftUI view and serves as the AUv3 factory
///
/// This implementation uses a safe pattern that prevents crashes during first render:
/// - Shows a placeholder view initially (safe to render without environment objects)
/// - Only switches to the real UI once paramTreeWrapper is configured
/// - Handles host timing issues where viewDidLoad may be called before configure
/// - Conforms to AUAudioUnitFactory to serve as the extension's principal class
public final class ModalAttractorsAUViewController: AUViewController, AUAudioUnitFactory {
    private var hostingController: NSHostingController<AnyView>?
    private var paramTreeWrapper: ParameterTree?
    private var audioUnit: ModalAttractorsExtensionAudioUnit?
    private var observation: NSKeyValueObservation?

    /// Configure the view controller with the parameter tree wrapper
    /// Call this before presenting to inject environment objects
    func configure(paramTreeWrapper: ParameterTree) {
        self.paramTreeWrapper = paramTreeWrapper
        if isViewLoaded { updateRootView() }
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        setupHostingControllerIfNeeded()
        updateRootView()
        NSLog("ModalAttractorsAUViewController viewDidLoad")
    }

    private func setupHostingControllerIfNeeded() {
        guard hostingController == nil else { return }

        // Create safe placeholder view (no environment object required)
        let placeholder = AnyView(
            VStack(spacing: 8) {
                Text("ModalAttractors")
                    .font(.title2).fontWeight(.bold)
                Text("Loading UI…")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            .padding(12)
        )

        let hc = NSHostingController(rootView: placeholder)
        hc.preferredContentSize = NSSize(
            width: UIConstants.Sizes.windowMinWidth,
            height: UIConstants.Sizes.windowMinHeight
        )

        hostingController = hc
        addChild(hc)
        view.addSubview(hc.view)

        hc.view.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            hc.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            hc.view.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            hc.view.topAnchor.constraint(equalTo: view.topAnchor),
            hc.view.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])
    }

    private func updateRootView() {
        guard let hc = hostingController else { return }
        guard let pt = paramTreeWrapper else { return }

        // Switch to the real UI now that we have the parameter tree
        hc.rootView = AnyView(
            ModalAttractorsExtensionMainView()
                .environmentObject(pt)
                .id("RootUI_v2") // bump to force refresh during dev
        )
    }

    // MARK: - AUAudioUnitFactory

    public func beginRequest(with context: NSExtensionContext) {
        // Extension lifecycle - no action needed
    }

    @objc
    public func createAudioUnit(with componentDescription: AudioComponentDescription) throws -> AUAudioUnit {
        audioUnit = try ModalAttractorsExtensionAudioUnit(componentDescription: componentDescription, options: [])

        guard let audioUnit = audioUnit as? ModalAttractorsExtensionAudioUnit else {
            throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioUnitErr_FailedInitialization))
        }

        // Setup parameter tree
        audioUnit.setupParameterTree(ModalAttractorsExtensionParameterSpecs.createAUParameterTree())

        // Get the parameter tree wrapper for UI
        if let paramTree = audioUnit.parameterTree {
            let wrapper = ParameterTree(auParameterTree: paramTree)
            self.paramTreeWrapper = wrapper

            // Configure the view with the wrapper
            configure(paramTreeWrapper: wrapper)

            // Observe parameter changes to ensure host can set initial values
            self.observation = audioUnit.observe(\.allParameterValues, options: [.new]) { object, change in
                guard let tree = audioUnit.parameterTree else { return }
                // This ensures the Audio Unit gets initial values from the host
                for param in tree.allParameters { param.value = param.value }
            }
        }

        return audioUnit
    }
}
