import SwiftUI

struct MainContentView: View {
  let webViewHub: WebViewHub
  init(_ viewAccessibleResources: ViewAccessibleResources) {
    self.webViewHub = WebViewHub(viewAccessibleResources)
  }

  var body: some View {
    VStack {
      LocalWebView { webViewIo in
        #if DEBUG
          webViewIo.loadURL("http://localhost:3000?debug=1")
        // let folder = WebViewHelper.getWebFolderPrioritized("www_dev", "www")
        // webViewIo.loadURL("app://\(folder)/index.html?debug=1")
        #else
          webViewIo.loadURL("app://www/index.html")
        #endif
        webViewHub.bindWebViewIo(webViewIo)
      }
    }.border(.green, width: 2).ignoresSafeArea()
  }
}
