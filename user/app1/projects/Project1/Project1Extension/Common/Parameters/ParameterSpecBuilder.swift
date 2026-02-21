import AudioToolbox

public class ParameterSpecBuilder<Address: RawRepresentable>
where Address.RawValue == AUParameterAddress {
  public init() {}
  public func Raw(
    address: Address,
    identifier: String,
    name: String,
    units: AudioUnitParameterUnit,
    valueRange: ClosedRange<AUValue>,
    defaultValue: AUValue,
    unitName: String? = nil,
    flags: AudioUnitParameterOptions = [
      AudioUnitParameterOptions.flag_IsWritable, AudioUnitParameterOptions.flag_IsReadable,
    ],
    valueStrings: [String]? = nil,
    dependentParameters: [NSNumber]? = nil
  ) -> ParameterSpec {
    return ParameterSpec(
      address: address.rawValue,
      identifier: identifier,
      name: name,
      units: units,
      valueRange: valueRange,
      defaultValue: defaultValue,
      unitName: unitName,
      flags: flags,
      valueStrings: valueStrings,
      dependentParameters: dependentParameters
    )
  }
  public func Linear(
    _ address: Address, _ identifier: String, _ name: String, _ defaultValue: AUValue,
    _ minValue: AUValue, _ maxValue: AUValue
  ) -> ParameterSpec {
    return ParameterSpec(
      address: address.rawValue,
      identifier: identifier,
      name: name,
      units: .generic,
      valueRange: minValue...maxValue,
      defaultValue: defaultValue,
    )
  }
  public func Unary(
    _ address: Address, _ identifier: String, _ name: String, _ defaultValue: AUValue
  ) -> ParameterSpec {
    return ParameterSpec(
      address: address.rawValue,
      identifier: identifier,
      name: name,
      units: .generic,
      valueRange: 0.0...1.0,
      defaultValue: defaultValue,
    )
  }
  public func Bool(
    _ address: Address, _ identifier: String, _ name: String, _ defaultValue: Bool
  ) -> ParameterSpec {
    return ParameterSpec(
      address: address.rawValue,
      identifier: identifier,
      name: name,
      units: .boolean,
      valueRange: 0.0...1.0,
      defaultValue: defaultValue ? 1.0 : 0.0,
    )
  }
  public func Enum(
    _ address: Address, _ identifier: String, _ name: String, _ defaultString: String,
    _ valueStrings: [String]
  ) -> ParameterSpec {
    let defaultIndex = valueStrings.firstIndex(of: defaultString) ?? 0
    return ParameterSpec(
      address: address.rawValue,
      identifier: identifier,
      name: name,
      units: .indexed,
      valueRange: 0.0...Float(valueStrings.count - 1),
      defaultValue: Float(defaultIndex),
      flags: [
        .flag_IsWritable, .flag_IsReadable,
        .flag_ValuesHaveStrings,
      ],
      valueStrings: valueStrings
    )
  }
}
