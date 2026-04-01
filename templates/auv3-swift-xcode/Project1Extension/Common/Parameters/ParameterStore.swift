import AVFoundation

final class ParameterStore: @unchecked Sendable {
  private var parameterValues: [AUValue] = []

  init() {}

  func setParameterCapacity(_ capacity: UInt32) {
    let clampedCapacity = min(Int(capacity), 4096)
    parameterValues = Array(repeating: 0, count: clampedCapacity)
  }

  func setParameter(_ address: AUParameterAddress, _ value: AUValue) {
    let index = Int(address)
    guard index >= 0, index < parameterValues.count else {
      return
    }
    parameterValues[index] = value
  }

  func getParameter(_ address: AUParameterAddress) -> AUValue {
    let index = Int(address)
    guard index >= 0, index < parameterValues.count else {
      return 0
    }
    return parameterValues[index]
  }
}
