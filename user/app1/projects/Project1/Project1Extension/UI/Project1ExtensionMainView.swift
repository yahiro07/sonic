//
//  Project1ExtensionMainView.swift
//  Project1Extension
//
//  Created by ore on 2026/02/21.
//

import SwiftUI

struct Project1ExtensionMainView: View {
  var parameterTree: ObservableAUParameterGroup

  var body: some View {
    ParameterSlider(param: parameterTree.global.gain)
  }
}
