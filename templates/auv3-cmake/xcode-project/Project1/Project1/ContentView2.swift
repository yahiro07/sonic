import AudioToolbox
import SwiftUI

struct ContentView2: View {
  let hostModel: AudioUnitHostModel
  @State private var isSheetPresented = false

  var body: some View {
    if let viewController = hostModel.viewModel.viewController {
      VStack {
        AUViewControllerUI(viewController: viewController)
      }
      //Note: Adding a border here prevents interaction with the AUViewControllerUI interior
      //Adding a border to the parent of a view spanning process boundaries seems to break hit testing
      //Occurs on macOS but not on iPad. Compatibility or compatibility issue between NSView and View?
      //Backgrounds and such are fine
      //.border(Color.red, width: 2) //BAD!!
      .background(Color.blue)
    } else {
      Text(hostModel.viewModel.message)
        .frame(minWidth: 400, minHeight: 200)
    }
  }
}

#Preview {
  ContentView2(hostModel: AudioUnitHostModel())
}
