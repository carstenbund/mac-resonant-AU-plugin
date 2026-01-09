//
//  ModalAttractorsAUViewController.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/8/26.
//

import CoreAudioKit
import SwiftUI

/// Custom AUViewController subclass that hosts the SwiftUI view
final class ModalAttractorsAUViewController: AUViewController {
    private var hostingController: NSViewController?

    /// Configure the view controller with the parameter tree wrapper
    /// Call this before presenting to inject environment objects
    func configure(paramTreeWrapper: ParameterTree) {
        let rootView = ModalAttractorsExtensionMainView()
            .environmentObject(paramTreeWrapper)

        let hc = NSHostingController(rootView: rootView)
        hc.preferredContentSize = NSSize(
            width: UIConstants.Sizes.windowMinWidth,
            height: UIConstants.Sizes.windowMinHeight
        )

        self.hostingController = hc
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        guard let hc = hostingController else { return }

        // Add hosting controller as child view controller
        addChild(hc)
        view.addSubview(hc.view)

        // Set up auto-layout constraints to fill the parent view
        hc.view.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            hc.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            hc.view.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            hc.view.topAnchor.constraint(equalTo: view.topAnchor),
            hc.view.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])

        //didMove(toParent: self)
    }
}
