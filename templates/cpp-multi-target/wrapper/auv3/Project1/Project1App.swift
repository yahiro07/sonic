import SwiftUI

@main
struct Project1App: App {
  private let hostModel = AudioUnitHostModel()

  var body: some Scene {
    WindowGroup {
      ContentView2(hostModel: hostModel)
    }
  }
}
