// enum LogKind {
//   case log
//   case mark
//   case warn
//   case error
//   case mute
//   case unmute
// }
import Foundation

struct LogItem {
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
  class LoggerCore {
    let udpLogger = UDPLogger()

    func pushLogItem(_ item: LogItem) {

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
  let loggerCore = LoggerCore()

  class LoggerEntry {

    private let subSystem: String

    init(subSystem: String) {
      self.subSystem = subSystem
    }

    private func pushLog(_ kind: String, _ message: String) {
      loggerCore.pushLogItem(
        LogItem(
          timestamp: Date().timeIntervalSince1970 * 1000, subSystem: subSystem, kind: kind,
          message: message
        ))
    }

    func log(_ message: String) {
      pushLog("log", message)
    }

    func mark(_ message: String) {
      pushLog("mark", message)
    }

    func warn(_ message: String) {
      pushLog("warn", message)
    }

    func error(_ message: String) {
      pushLog("error", message)
    }
  }

  let logger = LoggerEntry(subSystem: "ext")

#else

  class LoggerCore {
    init() {}
    func pushLogItem(_ item: LogItem) {
    }
  }
  let loggerCore = LoggerCore()

  class LoggerEntry {
    init() {}
    func log(_ message: String) {
    }
    func mark(_ message: String) {
    }
    func warn(_ message: String) {
    }
    func error(_ message: String) {
    }
  }
  let logger = LoggerEntry()
#endif
