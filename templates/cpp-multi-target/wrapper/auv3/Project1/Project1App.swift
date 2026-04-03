import SwiftUI

@main
struct Project1App: App {
  @Environment(\.scenePhase) private var scenePhase
  private let hostModel = AudioUnitHostModel()

  var body: some Scene {
    WindowGroup {
      #if os(iOS)
        ContentView2(hostModel: hostModel).ignoresSafeArea()  //fullscreen
      #else
        ContentView2(hostModel: hostModel)  //window with title bar
      #endif
    }.onChange(of: scenePhase) {
      switch scenePhase {
      case .background:
        hostModel.saveState()
      case .inactive:
        break
      case .active:
        break
      @unknown default:
        break
      }
    }
  }
}
