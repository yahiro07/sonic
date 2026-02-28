import Foundation

private let jsonFloatFormatter: NumberFormatter = {
  let formatter = NumberFormatter()
  formatter.locale = Locale(identifier: "en_US_POSIX")
  formatter.numberStyle = .decimal
  formatter.usesGroupingSeparator = false
  formatter.minimumFractionDigits = 0
  formatter.maximumFractionDigits = 5
  formatter.roundingMode = .halfUp
  return formatter
}()

//文字列化で小数点以下の桁数が無駄に多くならないようにする
func encodeFloatForJson(_ value: Float) -> NSDecimalNumber {
  if let str = jsonFloatFormatter.string(from: NSNumber(value: value)) {
    return NSDecimalNumber(string: str, locale: Locale(identifier: "en_US_POSIX"))
  }
  return NSDecimalNumber(value: Double(value))
}

func mapMessageFromUI_fromDictionary(_ dict: [String: Any]) -> MessageFromUI? {
  guard let type = dict["type"] as? String else { return nil }
  switch type {
  case "putLogItem":
    if let timeStamp = dict["timeStamp"] as? Double,
      let kind = dict["kind"] as? String,
      let message = dict["message"] as? String
    {
      return .putLogItem(timeStamp: timeStamp, kind: kind, message: message)
    }
  case "uiLoaded":
    return .uiLoaded
  case "beginParameterEdit":
    if let paramKey = dict["paramKey"] as? String {
      return .beginParameterEdit(paramKey: paramKey)
    }
  case "endParameterEdit":
    if let paramKey = dict["paramKey"] as? String {
      return .endParameterEdit(paramKey: paramKey)
    }
  case "setParameter":
    if let paramKey = dict["paramKey"] as? String,
      let value = dict["value"] as? Double
    {
      return .setParameter(paramKey: paramKey, value: Float(value))
    }
  case "loadFullParameters":
    if let parametersVersion = dict["parametersVersion"] as? Int,
      let parametersDict = dict["parameters"] as? [String: Double]
    {
      var parameters: [String: Float] = [:]
      for (key, value) in parametersDict {
        parameters[key] = Float(value)
      }
      return .loadFullParameters(parametersVersion: parametersVersion, parameters: parameters)
    }
  case "noteOnRequest":
    if let noteNumber = dict["noteNumber"] as? Int {
      return .noteOnRequest(noteNumber: noteNumber)
    }
  case "noteOffRequest":
    if let noteNumber = dict["noteNumber"] as? Int {
      return .noteOffRequest(noteNumber: noteNumber)
    }
  //
  case "rpcReadFileRequest":
    if let rpcId = dict["rpcId"] as? Int,
      let path = dict["path"] as? String,
      let skipIfNotExists = dict["skipIfNotExists"] as? Bool
    {
      return .rpcReadFileRequest(rpcId: rpcId, path: path, skipIfNotExists: skipIfNotExists)
    }
  case "rpcWriteFileRequest":
    if let rpcId = dict["rpcId"] as? Int,
      let path = dict["path"] as? String,
      let content = dict["content"] as? String,
      let append = dict["append"] as? Bool
    {
      return .rpcWriteFileRequest(rpcId: rpcId, path: path, content: content, append: append)
    }
  case "rpcDeleteFileRequest":
    if let rpcId = dict["rpcId"] as? Int,
      let path = dict["path"] as? String
    {
      return .rpcDeleteFileRequest(rpcId: rpcId, path: path)
    }
  case "rpcLoadStateKvsItems":
    if let rpcId = dict["rpcId"] as? Int {
      return .rpcLoadStateKvsItems(rpcId: rpcId)
    }
  case "writeStateKvsItem":
    if let key = dict["key"] as? String,
      let value = dict["value"] as? String
    {
      return .writeStateKvsItem(key: key, value: value)
    }
  case "deleteStateKvsItem":
    if let key = dict["key"] as? String {
      return .deleteStateKvsItem(key: key)
    }
  default:
    return nil
  }
  return nil
}

func mapMessageFromApp_toDictionary(_ msg: MessageFromApp) -> [String: Any]? {
  switch msg {
  case .setParameter(let paramKey, let value):
    return [
      "type": "setParameter",
      "paramKey": paramKey,
      "value": encodeFloatForJson(value),
    ]
  case .bulkSendParameters(let params):
    let encodedParams: [String: Any] = params.mapValues { encodeFloatForJson($0) }
    return ["type": "bulkSendParameters", "parameters": encodedParams]
  case .hostNoteOn(let noteNumber, let velocity):
    return [
      "type": "hostNoteOn",
      "noteNumber": noteNumber,
      "velocity": encodeFloatForJson(velocity),
    ]
  case .hostNoteOff(let noteNumber):
    return [
      "type": "hostNoteOff", "noteNumber": noteNumber,
    ]
  case .standaloneAppFlag:
    return [
      "type": "standaloneAppFlag"
    ]
  case .latestParametersVersion(let version):
    return [
      "type": "latestParametersVersion",
      "version": version,
    ]
  //
  case .rpcReadFileResponse(let rpcId, let success, let content):
    return [
      "type": "rpcReadFileResponse",
      "rpcId": rpcId,
      "success": success,
      "content": content,
    ]
  case .rpcWriteFileResponse(let rpcId, let success):
    return [
      "type": "rpcWriteFileResponse",
      "rpcId": rpcId,
      "success": success,
    ]
  case .rpcDeleteFileResponse(let rpcId, let success):
    return [
      "type": "rpcDeleteFileResponse",
      "rpcId": rpcId,
      "success": success,
    ]
  case .rpcLoadStateKvsItemsResponse(let rpcId, let items):
    return [
      "type": "rpcLoadStateKvsItemsResponse",
      "rpcId": rpcId,
      "items": items,
    ]
  }
}
