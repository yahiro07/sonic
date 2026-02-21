//
//  MySynth2App.swift
//  MySynth2
//
//  Created by ore on 2026/02/21.
//

import SwiftUI

@main
struct MySynth2App: App {
    let pluginWrapper = PlugingWrapper()
    init(){
        Greet()
        pluginWrapper.run()
    }
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}


class PlugingWrapper {
    var mySynth = MySynthesizer()
    func run(){
        mySynth.noteOn(48, 0.7)
    }
}
