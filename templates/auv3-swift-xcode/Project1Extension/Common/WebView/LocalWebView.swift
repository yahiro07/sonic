import Combine
import SwiftUI
import WebKit

private class ScriptMessageHandler: NSObject, WKScriptMessageHandler {
  let handler: (JsDataDictionary) -> Void

  init(handler: @escaping (JsDataDictionary) -> Void) {
    self.handler = handler
  }

  func userContentController(
    _ userContentController: WKUserContentController, didReceive message: WKScriptMessage
  ) {
    if let jsDataDictionary = message.body as? JsDataDictionary {
      handler(jsDataDictionary)
    } else {
      logger.warn("Invalid message body type: \(type(of: message.body))")
    }
  }
}

private func serializeDictionaryToJsonString(_ dict: [String: Any]) -> String {
  let data = try! JSONSerialization.data(withJSONObject: dict as Any, options: [])
  return String(data: data, encoding: .utf8)!
}

private func sendMessageToWebViewRaw(webView: WKWebView, jsDataDictionary: JsDataDictionary) {
  let jsStringifiedData = serializeDictionaryToJsonString(jsDataDictionary)
  // logger.log("sending message to UI: \(jsStringifiedData)")
  //window.pluginEditorCallback()を呼び出す
  let jsCode =
    "window.pluginEditorCallback && window.pluginEditorCallback(\(jsStringifiedData));"
  webView.evaluateJavaScript(jsCode)
}

class WebViewCoordinator: NSObject, WebViewIoProtocol, WKNavigationDelegate {
  weak var webView: WKWebView?
  private var receivers: [UUID: (JsDataDictionary) -> Void] = [:]
  private var didCallOnBind = false

  func callOnBindIfNeeded(_ onBind: (WebViewIoProtocol) -> Void) {
    guard !didCallOnBind else { return }
    didCallOnBind = true
    onBind(self)
  }

  func dispatchFromUI(_ dict: JsDataDictionary) {
    receivers.values.forEach { $0(dict) }
  }

  func loadURL(_ urlString: String) {
    logger.log("loadURL: \(urlString)")
    guard let webView else { return }
    if let url = URL(string: urlString) {
      webView.load(URLRequest(url: url))
    } else {
      logger.warn("Invalid URL string: \(urlString)")
    }
  }

  func sendRawMessageToUI(data: JsDataDictionary) {
    guard let webView else { return }
    sendMessageToWebViewRaw(webView: webView, jsDataDictionary: data)
  }

  @discardableResult
  func subscribeRawMessageFromUI(receiver: @escaping (JsDataDictionary) -> Void)
    -> AnyCancellable
  {
    let id = UUID()
    receivers[id] = receiver
    return AnyCancellable { [weak self] in
      self?.receivers.removeValue(forKey: id)
    }
  }

  func webView(
    _ webView: WKWebView,
    decidePolicyFor navigationAction: WKNavigationAction,
    decisionHandler: @escaping (WKNavigationActionPolicy) -> Void
  ) {
    if navigationAction.navigationType == .reload {
      logger.log("Detected WKWebView reload: \(String(describing: navigationAction.request.url))")
    }
    decisionHandler(.allow)
  }

  func webView(
    _ webView: WKWebView,
    didFailProvisionalNavigation navigation: WKNavigation!,
    withError error: Error
  ) {
    logger.error("WebView failed to load (provisional navigation): \(error.localizedDescription)")
    let nsError = error as NSError
    logger.error("Error domain: \(nsError.domain), code: \(nsError.code)")

    if nsError.domain == NSURLErrorDomain {
      switch nsError.code {
      case NSURLErrorCannotConnectToHost:
        logger.error("Cannot connect to host - server may not be running")
      case NSURLErrorNotConnectedToInternet:
        logger.error("No internet connection")
      case NSURLErrorBadURL:
        logger.error("Invalid URL")
      default:
        logger.error("Network error occurred")
      }
    }
  }

  func webView(
    _ webView: WKWebView,
    didFail navigation: WKNavigation!,
    withError error: Error
  ) {
    logger.error("WebView failed during navigation: \(error.localizedDescription)")
    let nsError = error as NSError
    logger.error("Error domain: \(nsError.domain), code: \(nsError.code)")
  }

  func webView(
    _ webView: WKWebView,
    didFinish navigation: WKNavigation!
  ) {
    logger.log("WebView finished loading: \(webView.url?.absoluteString ?? "unknown")")
  }

}

class MySchemeHandler: NSObject, WKURLSchemeHandler {

  func webView(
    _ webView: WKWebView,
    start urlSchemeTask: WKURLSchemeTask
  ) {
    logger.log("urlSchemeTask: \(urlSchemeTask.request.url?.absoluteString ?? "unknown")")

    guard let url = urlSchemeTask.request.url else { return }

    // print("request url:", url.absoluteString)
    // print("url.path:", url.path)

    let resourceURL = Bundle.main.resourceURL!
    let fileURL = resourceURL.appendingPathComponent("pages")
      .appendingPathComponent(url.host ?? "")
      .appendingPathComponent(url.path)

    // print("Loading: \(fileURL.path)")

    guard let data = try? Data(contentsOf: fileURL) else {
      logger.error("Failed to load file for URL: \(url), path: \(fileURL.path)")
      urlSchemeTask.didFailWithError(NSError(domain: "file", code: 404))
      return
    }

    let response = HTTPURLResponse(
      url: url,
      statusCode: 200,
      httpVersion: "HTTP/1.1",
      headerFields: [
        "Content-Type": mimeType(for: fileURL.path),
        "Content-Length": "\(data.count)",
      ]
    )!
    logger.log("response: \(response)")

    urlSchemeTask.didReceive(response)
    urlSchemeTask.didReceive(data)
    urlSchemeTask.didFinish()
  }

  func webView(
    _ webView: WKWebView,
    stop urlSchemeTask: WKURLSchemeTask
  ) {
  }

  private func mimeType(for path: String) -> String {
    if path.hasSuffix(".html") { return "text/html" }
    if path.hasSuffix(".js") { return "application/javascript" }
    if path.hasSuffix(".css") { return "text/css" }
    if path.hasSuffix(".json") { return "application/json" }
    return "application/octet-stream"
  }
}

func commonWebViewSetup(
  coordinator: WebViewCoordinator, onBind: @escaping (WebViewIoProtocol) -> Void
) -> WKWebView {
  let config = WKWebViewConfiguration()

  config.setURLSchemeHandler(MySchemeHandler(), forURLScheme: "app")

  let userContentController: WKUserContentController = WKUserContentController()
  userContentController.add(
    ScriptMessageHandler { [weak coordinator] dict in
      coordinator?.dispatchFromUI(dict)
    },
    name: "pluginEditor"
  )
  config.userContentController = userContentController

  let webView = WKWebView(frame: .zero, configuration: config)
  webView.isInspectable = true
  webView.navigationDelegate = coordinator

  coordinator.webView = webView

  coordinator.callOnBindIfNeeded(onBind)

  return webView
}

#if os(macOS)
  struct LocalWebView: NSViewRepresentable {
    let onBind: (WebViewIoProtocol) -> Void

    init(_ onBind: @escaping (WebViewIoProtocol) -> Void) {
      self.onBind = onBind
    }

    func makeCoordinator() -> WebViewCoordinator { WebViewCoordinator() }

    func makeNSView(context: Context) -> WKWebView {
      return commonWebViewSetup(coordinator: context.coordinator, onBind: onBind)
    }

    func updateNSView(_ webView: WKWebView, context: Context) {
    }

  }

#elseif os(iOS)
  // Use UIViewRepresentable on iOS
  struct LocalWebView: UIViewRepresentable {

    let onBind: (WebViewIoProtocol) -> Void

    init(_ onBind: @escaping (WebViewIoProtocol) -> Void) {
      self.onBind = onBind
    }

    func makeCoordinator() -> WebViewCoordinator { WebViewCoordinator() }

    func makeUIView(context: Context) -> WKWebView {
      let webView = commonWebViewSetup(coordinator: context.coordinator, onBind: onBind)
      //remove top and bottom insets
      webView.scrollView.contentInsetAdjustmentBehavior = .never
      return webView
    }

    func updateUIView(_ webView: WKWebView, context: Context) {
    }

  }
#endif

class WebViewHelper {
  static func getWebFolderPrioritized(
    _ devFolderName: String, _ prodFolderName: String
  ) -> String {
    let resourceURL = Bundle.main.resourceURL!
    let devURL = resourceURL.appendingPathComponent(devFolderName).appendingPathComponent(
      "index.html")

    if FileManager.default.fileExists(atPath: devURL.path) {
      return devFolderName
    } else {
      return prodFolderName
    }
  }
}
