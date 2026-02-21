//
//  MySynth2App.swift
//  MySynth2
//
//  Created by ore on 2026/02/21.
//

import AudioToolbox
import SwiftUI

@main
struct MySynth2App: App {
  let pluginWrapper = PluginWrapper()
  init() {
    Greet()
    pluginWrapper.run()
  }
  var body: some Scene {
    WindowGroup {
      ContentView()
    }
  }
}

class PluginWrapper {
  var mySynth = MySynthesizer()
  var parameterBuilder = ParameterBuilderImpl()
  func run() {
    mySynth.noteOn(48, 0.7)
    let baseBuilder: UnsafeMutablePointer<ParameterBuilder> =
      parameterBuilder.asParameterBuilder()!
    mySynth.setupParameters(&baseBuilder.pointee)

    let pt = ParameterTreeSpec {
      ParameterGroupSpec(identifier: "global", name: "Global") {
        parameterBuilder.getItems().map { item in
          ParameterSpec(
            address: item.address,
            identifier: String(item.identifier),
            name: String(item.label),
            units: item.type == .Enum ? .indexed : (item.type == .Bool ? .boolean : .generic),
            valueRange: item.minValue...item.maxValue,
            defaultValue: item.defaultValue,
            unitName: nil,
            flags: item.type == .Enum
              ? [
                .flag_IsWritable, .flag_IsReadable,
                .flag_ValuesHaveStrings,
              ] : [.flag_IsWritable, .flag_IsReadable],
            valueStrings: item.valueStrings.empty()
              ? nil : item.valueStrings.map { String($0) },
            dependentParameters: nil
          )
        }
      }
    }
    print(pt)
  }
}
