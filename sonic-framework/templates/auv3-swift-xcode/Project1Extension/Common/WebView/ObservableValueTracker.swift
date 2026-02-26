import Observation

@MainActor
final class ObservableValueTracker {
  private var onEmit: (@MainActor (_ key: String, _ value: Float) -> Void)?
  private var pendingEchoSuppressions: [String: Float] = [:]
  private let echoSuppressionEpsilon: Float = 1.0e-6

  func setReceiver(onEmit: @escaping @MainActor (_ key: String, _ value: Float) -> Void) {
    self.onEmit = onEmit
  }

  func reserveEchoSuppression(paramKey: String, value: Float) {
    pendingEchoSuppressions[paramKey] = value
  }

  private func consumeEchoSuppressionIfPresent(paramKey: String, observedValue: Float) -> Bool {
    guard let expectedValue = pendingEchoSuppressions.removeValue(forKey: paramKey) else {
      return false
    }
    return abs(observedValue - expectedValue) <= echoSuppressionEpsilon
  }

  func trackParameterValue(paramKey: String, paramEntry: ObservableAUParameter) {
    withObservationTracking { [weak paramEntry] in
      _ = paramEntry?.value
    } onChange: { [weak self, weak paramEntry] in
      guard let self, let paramEntry else { return }
      Task { @MainActor in
        let newValue = paramEntry.value
        if !self.consumeEchoSuppressionIfPresent(paramKey: paramKey, observedValue: newValue) {
          self.onEmit?(paramKey, newValue)
        }

        // Re-arm tracking for the next change.
        self.trackParameterValue(paramKey: paramKey, paramEntry: paramEntry)
      }
    }
  }
}
