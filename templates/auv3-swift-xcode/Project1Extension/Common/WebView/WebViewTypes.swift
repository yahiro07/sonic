import Combine

enum MessageFromUI {
  case putLogItem(timestamp: Double, logKind: String, message: String)
  //UI読み込み完了時の通知,このあとプラグイン本体から初期パラメタを送信する
  case uiLoaded
  //UI操作で変更されたパラメタをプラグイン本体に送信
  case beginParameterEdit(paramKey: String)
  case endParameterEdit(paramKey: String)
  case setParameter(paramKey: String, value: Float)
  //UIからパラメタセットを受け取り,必要ならマイグレーションを適用してparameterTreeに反映する,プリセットロード用
  case loadFullParameters(parametersVersion: Int, parameters: [String: Float])
  //UIに含まれる鍵盤などからのプラグイン本体に送るノートオンオフ要求
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
  //ホストやプラグイン本体で変更されたパラメタをUIに送信
  case setParameter(paramKey: String, value: Float)
  case bulkSendParameters(params: [String: Float])
  //ホストから送られたノートをUI側で受け取るメッセージ
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
