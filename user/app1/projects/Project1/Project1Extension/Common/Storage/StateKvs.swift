public class StateKvs {
  private(set) var items: [String: String] = [:]

  func setItems(_ items: [String: String]) {
    self.items = items
  }

  func read(_ key: String) -> String? {
    let value = items[key]
    // logger.log("StateKvs read key: \(key) value: \(value ?? "nil")")
    return value
  }

  func write(_ key: String, _ value: String) {
    // logger.log("StateKvs write key: \(key) value: \(value)")
    items[key] = value
  }

  func delete(_ key: String) {
    // logger.log("StateKvs delete key: \(key)")
    items.removeValue(forKey: key)
  }
}
