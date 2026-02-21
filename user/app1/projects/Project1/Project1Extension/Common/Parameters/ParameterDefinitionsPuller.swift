import AudioToolbox
import Foundation

func pullParameterDefinitionsFromCppSide(_ synthInstanceHandle: SynthInstanceHandle)
  -> ParameterTreeSpec
{
  var parameterBuilder = ParameterBuilderImpl()
  parameterBuilder.callSetupParameters(synthInstanceHandle)

  let parameterSpecs = ParameterTreeSpec {
    ParameterGroupSpec(
      //nesting is not supported, all parameters are flattened into a single group
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

extension ParameterGroupSpec {
  init(identifier: String, name: String, children: [NodeSpec]) {
    self.init(identifier: identifier, name: name) {
      return children
    }
  }
}
