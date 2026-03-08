//
//  Project1App.swift
//  Project1
//
//  Created by ore on 2026/03/08.
//

import SwiftUI

@main
struct Project1App: App {
    private let hostModel = AudioUnitHostModel()

    var body: some Scene {
        WindowGroup {
            ContentView(hostModel: hostModel)
        }
    }
}
