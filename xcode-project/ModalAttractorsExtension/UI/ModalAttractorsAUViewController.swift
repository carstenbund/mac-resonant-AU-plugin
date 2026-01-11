//
//  ModalAttractorsAUViewController.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import CoreAudioKit
import SwiftUI
import AppKit

/// Custom AUViewController subclass that hosts the SwiftUI view
///
/// This implementation uses a safe pattern that prevents crashes during first render:
/// - Shows a placeholder view initially (safe to render without environment objects)
/// - Only switches to the real UI once paramTreeWrapper is configured
/// - Handles host timing issues where viewDidLoad may be called before configure
final class ModalAttractorsAUViewController: AUViewController {
    private var hostingController: NSHostingController<AnyView>?
    private var paramTreeWrapper: ParameterTree?

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
}
