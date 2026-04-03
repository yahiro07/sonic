import SwiftUI

struct MainContentView: View {
  let webViewHub: WebViewHub
  init(_ viewAccessibleResources: ViewAccessibleResources) {
    self.webViewHub = WebViewHub(viewAccessibleResources)
  }

  var body: some View {
    VStack {
      LocalWebView { webViewIo in
        // webViewIo.loadURL("http://localhost:3000?debug=1")
        webViewIo.loadURL("app://www-bundles/index.html")
        webViewHub.bindWebViewIo(webViewIo)
      }
    }
    //.border(.green, width: 2)
    .ignoresSafeArea()
  }
}
