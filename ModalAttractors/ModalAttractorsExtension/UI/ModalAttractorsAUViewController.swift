//
//  ModalAttractorsAUViewController.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import CoreAudioKit
import SwiftUI

/// Custom AUViewController subclass that hosts the SwiftUI view
/// Resilient to configure() / viewDidLoad() timing issues
final class ModalAttractorsAUViewController: AUViewController {
    private var hostingController: NSHostingController<ModalAttractorsExtensionMainView>!
    private var paramTreeWrapper: ParameterTree?

    /// Configure the view controller with the parameter tree wrapper
    /// Can be called before or after viewDidLoad()
    func configure(paramTreeWrapper: ParameterTree) {
        self.paramTreeWrapper = paramTreeWrapper

        // If view is already loaded, update the environment object
        if isViewLoaded {
            updateRootView()
        }
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        // Create hosting controller immediately (even if paramTree not ready yet)
        // This prevents empty view / generic UI fallback
        setupHostingController()
    }

    private func setupHostingController() {
        // Create root view
        let rootView = ModalAttractorsExtensionMainView()

        // Create hosting controller
        hostingController = NSHostingController(rootView: rootView)
        hostingController.preferredContentSize = NSSize(
            width: UIConstants.Sizes.windowMinWidth,
            height: UIConstants.Sizes.windowMinHeight
        )

        // If we already have the parameter tree, inject it now
        if let paramTree = paramTreeWrapper {
            updateRootView()
        }

        // Add hosting controller as child view controller
        addChild(hostingController)
        view.addSubview(hostingController.view)

        // Set up auto-layout constraints to fill the parent view
        hostingController.view.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            hostingController.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            hostingController.view.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            hostingController.view.topAnchor.constraint(equalTo: view.topAnchor),
            hostingController.view.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])
    }

    private func updateRootView() {
        guard let paramTree = paramTreeWrapper else { return }

        // Update the root view with the parameter tree
        let rootView = ModalAttractorsExtensionMainView()
            .environmentObject(paramTree)

        hostingController.rootView = rootView
    }
}
