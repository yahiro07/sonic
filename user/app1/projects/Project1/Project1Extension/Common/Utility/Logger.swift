// enum LogKind {
//   case log
//   case mark
//   case warn
//   case error
//   case mute
//   case unmute
// }
import Foundation

public struct LogItem {
  let timestamp: Double  //ms from epoch
  let subSystem: String
  let kind: String
  let message: String
}

//00:00:00.000
func formatTimestamp(_ timestamp: Double) -> String {
  let date = Date(timeIntervalSince1970: timestamp / 1000)
  let formatter = DateFormatter()
  formatter.dateFormat = "HH:mm:ss.SSS"
  return formatter.string(from: date)
}

let subSystemIcons: [String: String] = [
  "host": "🟣",
  "ext": "🔸",
  "ui": "🔹",
  "dsp": "🔺",
]

let logKindIcons: [String: String] = [
  "log": "",
  "mark": "🔽",
  "warn": "⚠️",
  "error": "📛",
]

#if DEBUG
  public class LoggerCore {
    let udpLogger = UDPLogger()

    public func pushLogItem(_ item: LogItem) {

      let ts = formatTimestamp(item.timestamp)
      let ssIcon = subSystemIcons[item.subSystem] ?? ""
      let kindIcon = logKindIcons[item.kind] ?? ""

      let logLine = "\(ts) [\(ssIcon)\(item.subSystem)] \(kindIcon) \(item.message)"
      if true {
        print(logLine)
      }
      if true {
        udpLogger.pushLogItem(item)
      }
    }
  }
  public let loggerCore = LoggerCore()

  public class LoggerEntry {

    private let subSystem: String

    public init(subSystem: String) {
      self.subSystem = subSystem
    }

    private func pushLog(_ kind: String, _ message: String) {
      loggerCore.pushLogItem(
        LogItem(
          timestamp: Date().timeIntervalSince1970 * 1000, subSystem: subSystem, kind: kind,
          message: message
        ))
    }

    public func log(_ message: String) {
      pushLog("log", message)
    }

    public func mark(_ message: String) {
      pushLog("mark", message)
    }

    public func warn(_ message: String) {
      pushLog("warn", message)
    }

    public func error(_ message: String) {
      pushLog("error", message)
    }
  }

  public let logger = LoggerEntry(subSystem: "ext")

#else

  public class LoggerCore {
    public init() {}
    public func pushLogItem(_ item: LogItem) {
    }
  }
  public let loggerCore = LoggerCore()

  public class LoggerEntry {
    public init() {}
    public func log(_ message: String) {
    }
    public func mark(_ message: String) {
    }
    public func warn(_ message: String) {
    }
    public func error(_ message: String) {
    }
  }
  public let logger = LoggerEntry()
#endif
