import Foundation

protocol StorageFileIO {
  func readFile(path: String, skipIfNotExist: Bool?) throws -> String
  func writeFile(path: String, content: String, append: Bool?) throws
  func deleteFile(path: String) throws
}

class SharedContainer {
  static private var appGroupId = ""

  static func setAppGroupId(_ appGroupId: String) {
    self.appGroupId = appGroupId
  }

  static func baseURL() throws -> URL {
    guard !appGroupId.isEmpty else {
      throw NSError(
        domain: "SharedContainer",
        code: 0,
        userInfo: [NSLocalizedDescriptionKey: "appGroupId is empty"])
    }
    guard
      let url = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: appGroupId)
    else {
      throw NSError(
        domain: "SharedContainer",
        code: 1,
        userInfo: [
          NSLocalizedDescriptionKey:
            "containerURL is nil. Check App Group entitlement: \(appGroupId)"
        ]
      )
    }
    return url
  }

  static func ensureSubFolder(_ folderName: String) throws -> URL {
    let fm = FileManager.default
    let folderURL = try baseURL().appendingPathComponent(folderName, isDirectory: true)
    var isDir: ObjCBool = false
    if fm.fileExists(atPath: folderURL.path, isDirectory: &isDir), !isDir.boolValue {
      try fm.removeItem(at: folderURL)
    }
    try fm.createDirectory(at: folderURL, withIntermediateDirectories: true, attributes: nil)
    return folderURL
  }

  static func getRelativePathFileURL(_ path: String) throws -> URL {
    let trimmed = path.trimmingCharacters(in: .whitespacesAndNewlines)
    if trimmed.isEmpty { throw NSError(domain: "SharedContainer", code: 2) }
    if trimmed.hasPrefix("/") { throw NSError(domain: "SharedContainer", code: 3) }
    if trimmed.hasSuffix("/") { throw NSError(domain: "SharedContainer", code: 4) }

    let parts = path.split(separator: "/")
    if parts.contains("..") || parts.contains(".") {
      throw NSError(domain: "SharedContainer", code: 5)
    }
    guard let fileNamePart = parts.last, !fileNamePart.isEmpty else {
      throw NSError(domain: "SharedContainer", code: 6)
    }
    let fileName = String(fileNamePart)

    let folderPart = parts.dropLast().joined(separator: "/")
    let folderURL = try ensureSubFolder(folderPart)
    return folderURL.appendingPathComponent(fileName, isDirectory: false)
  }
}

class StorageFileIOImpl: StorageFileIO {

  func debugLogDataLocation() throws {
    do {
      let baseURL = try SharedContainer.baseURL()
      logger.log("data location: \(baseURL.path)")
    } catch {
      logger.error("debugLogDataLocation failed: \(error)")
    }
  }

  func readFile(path: String, skipIfNotExist: Bool? = false) throws -> String {
    let fileURL = try SharedContainer.getRelativePathFileURL(path)
    if FileManager.default.fileExists(atPath: fileURL.path) {
      let content = try String(contentsOf: fileURL, encoding: .utf8)
      // logger.log("PresetFilesIO readFile path: \(path) content: \(content)")
      return content
    } else if skipIfNotExist == true {
      return ""
    } else {
      throw NSError(
        domain: "PresetFilesIO", code: 404, userInfo: [NSLocalizedDescriptionKey: "file not found"])
    }
  }

  func writeFile(path: String, content: String, append: Bool? = false) throws {
    let fileURL = try SharedContainer.getRelativePathFileURL(path)
    let fm = FileManager.default

    if append == true && fm.fileExists(atPath: fileURL.path) {
      let data = Data(content.utf8)
      let handle = try FileHandle(forWritingTo: fileURL)
      handle.seekToEndOfFile()
      handle.write(data)
      handle.closeFile()
      return
    }
    try content.write(to: fileURL, atomically: true, encoding: .utf8)
    // logger.log("PresetFilesIO writeFile path: \(path) content: \(content)")
  }

  func deleteFile(path: String) throws {
    let fileURL = try SharedContainer.getRelativePathFileURL(path)
    try FileManager.default.removeItem(at: fileURL)
    // logger.log("PresetFilesIO deleteFile path: \(path)")
  }
}
