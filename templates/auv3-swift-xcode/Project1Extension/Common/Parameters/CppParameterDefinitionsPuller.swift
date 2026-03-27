import AudioToolbox
import Foundation

func mapParameterFlags(_ flags: ParameterFlags, _ itemType: CppParameterType)
  -> AudioUnitParameterOptions
{
  var resFlags: AudioUnitParameterOptions = []

  if itemType == .Enum {
    resFlags.insert(.flag_ValuesHaveStrings)
  }
  if flags.rawValue & ParameterFlags.IsHidden.rawValue == 0 {
    resFlags.insert(.flag_IsReadable)

    if flags.rawValue & ParameterFlags.IsReadOnly.rawValue == 0 {
      resFlags.insert(.flag_IsWritable)
    }
  }

  if flags.rawValue & ParameterFlags.NonAutomatable.rawValue == 0 {
    resFlags.insert(.flag_CanRamp)
  }
  return resFlags
}

func pullParameterDefinitionsFromCppSide(_ synthInstanceHandle: SynthInstanceHandle)
  -> ParameterTreeSpec
{
  var parameterBuilder = CppParameterBuilderImpl()
  parameterBuilder.callSetupParameters(synthInstanceHandle)

  let parameterSpecs = ParameterTreeSpec {
    ParameterGroupSpec(
      //currently nesting is not supported, all parameters are flattened into a single group
      identifier: "global",
      name: "Global",
      children: parameterBuilder.getItems().map { item in
        ParameterSpec(
          address: UInt64(item.id),
          identifier: String(item.paramKey),
          name: String(item.label),
          units: item.type == .Enum ? .indexed : (item.type == .Bool ? .boolean : .generic),
          valueRange: Float(item.minValue)...Float(item.maxValue),
          defaultValue: Float(item.defaultValue),
          unitName: nil,
          flags: mapParameterFlags(item.flags, item.type),
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
