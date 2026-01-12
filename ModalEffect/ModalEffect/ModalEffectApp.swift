//
//  ModalEffectApp.swift
//  ModalEffect
//
//  Created by Carsten on 1/7/26.
//

import SwiftUI

@main
struct ModalEffectApp: App {
    @State private var hostModel = AudioUnitHostModel()

    var body: some Scene {
        WindowGroup {
            ContentView(hostModel: hostModel)
        }
    }
}
