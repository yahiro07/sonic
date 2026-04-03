import SwiftUI

@main
struct Project1App: App {
  private let hostModel = AudioUnitHostModel()

  var body: some Scene {
    WindowGroup {
      #if os(iOS)
        ContentView2(hostModel: hostModel).ignoresSafeArea()  //fullscreen
      #else
        ContentView2(hostModel: hostModel)  //window with title bar
      #endif
    }
  }
}
