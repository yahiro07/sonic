// enum LogKind {
//   case trace
//   case info
//   case log
//   case warn
//   case error
// }
import Foundation

struct LogItem {
  let timestamp: Double  //ms from epoch
  let subsystem: String
  let logKind: String
  let message: String
}

//00:00:00.000
func formatTimestamp(_ timestamp: Double) -> String {
  let date = Date(timeIntervalSince1970: timestamp / 1000)
  let formatter = DateFormatter()
  formatter.dateFormat = "HH:mm:ss.SSS"
  return formatter.string(from: date)
}

let subsystemIcons: [String: String] = [
  "host": "🧊",
  "ext": "🔸",
  "ui": "🔹",
  "dsp": "🔺",
]

let logKindIcons: [String: String] = [
  "trace": "🔽",
  "info": "◻️",
  "log": "▫️",
  "warn": "⚠️",
  "error": "📛",
]

#if DEBUG
  class LoggerCore {
    let udpLogger = UDPLogger()

    func printLogLine(_ item: LogItem) {
      let ts = formatTimestamp(item.timestamp)
      let ssIcon = subsystemIcons[item.subsystem] ?? ""
      let kindIcon = logKindIcons[item.logKind] ?? ""
      let logLine = "\(ts) [\(ssIcon)\(item.subsystem)] \(kindIcon) \(item.message)"
      print(logLine)
    }
    func pushLogItem(_ item: LogItem) {
      printLogLine(item)

      let timestamp = item.timestamp
      let subsystem = item.subsystem
      let logKind = item.logKind
      let message = item.message.replacingOccurrences(of: "\"", with: "\\\"")
      let jsonText =
        "{ \"timestamp\": \(timestamp), \"subsystem\": \"\(subsystem)\", \"logKind\": \"\(logKind)\", \"message\": \"\(message)\"}"
      udpLogger.pushLogLine(jsonText)
    }
  }
  let loggerCore = LoggerCore()

  class LoggerEntry {

    private let subsystem: String

    init(subsystem: String) {
      self.subsystem = subsystem
    }

    private func pushLog(_ logKind: String, _ message: String) {
      let item = LogItem(
        timestamp: Date().timeIntervalSince1970 * 1000, subsystem: subsystem, logKind: logKind,
        message: message
      )
      loggerCore.pushLogItem(item)
    }

    func trace(_ message: String) {
      pushLog("trace", message)
    }

    func info(_ message: String) {
      pushLog("info", message)
    }

    func log(_ message: String) {
      pushLog("log", message)
    }

    func warn(_ message: String) {
      pushLog("warn", message)
    }

    func error(_ message: String) {
      pushLog("error", message)
    }
  }

  let logger = LoggerEntry(subsystem: "ext")

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
