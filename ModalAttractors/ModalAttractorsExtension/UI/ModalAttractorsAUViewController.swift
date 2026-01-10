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
    private var hostingController: NSHostingController<AnyView>!
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
        // CRITICAL: Create root view with environment object if available
        // Creating the view without the required @EnvironmentObject causes SwiftUI to fail,
        // which results in the host showing a fallback default parameter tree view
        let rootView: AnyView

        if let paramTree = paramTreeWrapper {
            // Create hosting controller with environment object from the start
            rootView = AnyView(ModalAttractorsExtensionMainView().environmentObject(paramTree))
        } else {
            // Fallback: create without environment object (view will fail until updateRootView is called)
            rootView = AnyView(ModalAttractorsExtensionMainView())
        }

        hostingController = NSHostingController(rootView: rootView)
        hostingController.preferredContentSize = NSSize(
            width: UIConstants.Sizes.windowMinWidth,
            height: UIConstants.Sizes.windowMinHeight
        )

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
        let rootView = AnyView(
            ModalAttractorsExtensionMainView()
                .environmentObject(paramTree)
        )

        hostingController.rootView = rootView 
    }
}
