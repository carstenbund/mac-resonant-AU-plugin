//
//  ModalAttractorsApp.swift
//  ModalAttractors
//
//  Created by Carsten on 1/7/26.
//

import SwiftUI

@main
struct ModalAttractorsApp: App {
    @State private var hostModel = AudioUnitHostModel()

    init() {
        AULogger.app.info("🚀 ModalAttractors app starting up...")
        AULogger.app.notice("📱 App initialized successfully")
    }

    var body: some Scene {
        WindowGroup {
            ContentView(hostModel: hostModel)
        }
    }
}
