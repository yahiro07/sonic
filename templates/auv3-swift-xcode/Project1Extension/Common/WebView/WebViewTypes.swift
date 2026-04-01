import Combine

enum MessageFromUI {
  case putLogItem(timestamp: Double, logKind: String, message: String)
  //Notification when the UI has finished loading; the plugin will then send the initial parameters
  case uiLoaded
  //Send parameters changed via UI operations to the plugin
  case beginParameterEdit(paramKey: String)
  case endParameterEdit(paramKey: String)
  case setParameter(paramKey: String, value: Float)
  //Receives a parameter set from the UI, applies migrations if necessary, and updates the parameterTree; for loading presets
  case loadFullParameters(parametersVersion: Int, parameters: [String: Float])
  //Note on/off requests sent from the keyboard and other UI elements to the plugin
  case noteOnRequest(noteNumber: Int)
  case noteOffRequest(noteNumber: Int)
  //
  case rpcReadFileRequest(rpcId: Int, path: String, skipIfNotExists: Bool)
  case rpcWriteFileRequest(rpcId: Int, path: String, content: String, append: Bool)
  case rpcDeleteFileRequest(rpcId: Int, path: String)
  case rpcLoadStateKvsItems(rpcId: Int)
  case writeStateKvsItem(key: String, value: String)
  case deleteStateKvsItem(key: String)
}

enum MessageFromApp {
  //Send parameters modified by the host or the plugin itself to the UI
  case setParameter(paramKey: String, value: Float)
  case bulkSendParameters(params: [String: Float])
  //A message represents notes sent from the host
  case hostNoteOn(noteNumber: Int, velocity: Float)
  case hostNoteOff(noteNumber: Int)
  case standaloneAppFlag
  case latestParametersVersion(version: Int)
  //
  case rpcReadFileResponse(rpcId: Int, success: Bool, content: String)
  case rpcWriteFileResponse(rpcId: Int, success: Bool)
  case rpcDeleteFileResponse(rpcId: Int, success: Bool)
  case rpcLoadStateKvsItemsResponse(rpcId: Int, items: [String: String])
}

typealias JsDataDictionary = [String: Any]

protocol WebViewIoProtocol {
  func loadURL(_ urlString: String)

  func sendRawMessageToUI(data: JsDataDictionary)
  @discardableResult
  func subscribeRawMessageFromUI(receiver: @escaping (JsDataDictionary) -> Void)
    -> AnyCancellable
}
