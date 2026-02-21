//
//  Parameters.swift
//  Project1Extension
//
//  Created by ore on 2026/02/21.
//

import AudioToolbox
import Foundation

extension ParameterGroupSpec {
  init(identifier: String, name: String, children: [NodeSpec]) {
    self.init(identifier: identifier, name: name) {
      return children
    }
  }
}

// let debugFallbackParameterSpecs = ParameterTreeSpec {
//   ParameterGroupSpec(identifier: "global", name: "Global") {
//     ParameterSpec(
//       // address: .gain,
//       address: 0,
//       identifier: "gain",
//       name: "Output Gain",
//       units: .linearGain,
//       valueRange: 0.0...1.0,
//       defaultValue: 0.25
//     )
//   }
// }

func createProject1ExtensionParameterSpecs(_ synthInstanceHandle: SynthInstanceHandle)
  -> ParameterTreeSpec
{
  if false {
    return ParameterTreeSpec {
      ParameterGroupSpec(identifier: "global", name: "Global") {
        ParameterSpec(
          // address: .gain,
          address: 0,
          identifier: "gain",
          name: "Output Gain",
          units: .linearGain,
          valueRange: 0.0...1.0,
          defaultValue: 0.25
        )
      }
    }
  } else {

    var parameterBuilder = ParameterBuilderImpl()
    parameterBuilder.callSetupParameters(synthInstanceHandle)

    let parameterSpecs = ParameterTreeSpec {
      ParameterGroupSpec(
        identifier: "global",
        name: "Global",
        children: parameterBuilder.getItems().map { item in
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
        })
    }
    print(parameterSpecs)
    return parameterSpecs
  }
}
