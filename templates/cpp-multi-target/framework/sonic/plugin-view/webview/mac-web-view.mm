#import "./mac-web-view.h"
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#endif
#include <Foundation/Foundation.h>
#import <WebKit/WebKit.h>
#include <dlfcn.h>
#include <functional>
#include <mach-o/dyld.h>
#include <string>

#if !__has_feature(objc_arc)
#error                                                                         \
    "mac-web-view.mm requires ARC. Enable -fobjc-arc for Objective-C++ sources."
#endif

using namespace sonic;

@interface AppSchemeHandler : NSObject <WKURLSchemeHandler>
@property(nonatomic, strong) NSString *rootPath;
@end

@implementation AppSchemeHandler

- (NSString *)mimeTypeForPath:(NSString *)path {
  NSString *extension = [path pathExtension];
  if ([extension isEqualToString:@"html"]) {
    return @"text/html";
  } else if ([extension isEqualToString:@"js"]) {
    return @"application/javascript";
  } else if ([extension isEqualToString:@"css"]) {
    return @"text/css";
  } else if ([extension isEqualToString:@"json"]) {
    return @"application/json";
  } else {
    return @"application/octet-stream";
  }
}

- (void)webView:(WKWebView *)webView
    startURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask {
  NSURL *url = urlSchemeTask.request.URL;

  NSString *host = url.host;
  NSString *relativePath = url.path;

  if ([relativePath hasPrefix:@"/"]) {
    relativePath = [relativePath substringFromIndex:1];
  }
  NSString *fullPath = [self.rootPath stringByAppendingPathComponent:host];
  fullPath = [fullPath stringByAppendingPathComponent:relativePath];

  NSData *data = [NSData dataWithContentsOfFile:fullPath];
  if (!data) {
    [urlSchemeTask didFailWithError:[NSError errorWithDomain:@"AppScheme"
                                                        code:404
                                                    userInfo:nil]];
    return;
  }

  NSString *mimeType = [self mimeTypeForPath:fullPath];
  NSHTTPURLResponse *response = [[NSHTTPURLResponse alloc]
       initWithURL:url
        statusCode:200
       HTTPVersion:@"HTTP/1.1"
      headerFields:@{
        @"Content-Type" : mimeType,
        @"Content-Length" : [NSString stringWithFormat:@"%lu", data.length],
      }];
  [urlSchemeTask didReceiveResponse:response];
  [urlSchemeTask didReceiveData:data];
  [urlSchemeTask didFinish];
}

- (void)webView:(WKWebView *)webView
    stopURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask {
}
@end

@interface ScriptMessageHandler : NSObject <WKScriptMessageHandler>
@property(nonatomic, copy) void (^onMessage)(id _Nullable body);
@end

@implementation ScriptMessageHandler
- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)message {
  if (self.onMessage) {
    self.onMessage(message.body);
  }
}
@end

@interface PluginWKWebView : WKWebView
@end

@implementation PluginWKWebView
#if !TARGET_OS_IPHONE
// Send the first click directly to the WebView's internal component
- (BOOL)acceptsFirstMouse:(NSEvent *)event {
  return YES;
}

// Request focus immediately after being attached to the plugin host window
- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  if (self.window) {
    [self.window setAcceptsMouseMovedEvents:YES];
    [self.window makeFirstResponder:self];
  }
}
#endif

@end

static NSString *ThisModuleDirPath() {
  Dl_info info{};
  if (dladdr((const void *)&ThisModuleDirPath, &info) == 0 || !info.dli_fname) {
    return nil;
  }
  char resolved[PATH_MAX];
  if (!realpath(info.dli_fname, resolved))
    return nil;

  NSString *modulePath = [NSString stringWithUTF8String:resolved];
  return [modulePath stringByDeletingLastPathComponent];
}

static NSString *PagesRootPath() {
  NSBundle *bundle = [NSBundle mainBundle];
  NSString *resourcePath = bundle.resourcePath;
  if (resourcePath.length > 0) {
    NSString *pagesPath =
        [resourcePath stringByAppendingPathComponent:@"pages"];
    if ([[NSFileManager defaultManager] fileExistsAtPath:pagesPath]) {
      return pagesPath;
    }
  }

  NSString *baseDir = ThisModuleDirPath();
  if (!baseDir) {
    return nil;
  }

  NSString *fallbackPath =
      [baseDir stringByAppendingPathComponent:@"../Resources/pages"];
  if ([[NSFileManager defaultManager] fileExistsAtPath:fallbackPath]) {
    return fallbackPath;
  }

  return nil;
}

class MacWebView::Impl {
public:
  WKWebView *webView = nil;
  id scriptHandler = nil;
  std::function<void(const std::string &)> messageReceiver;
};

MacWebView::MacWebView() : impl(new Impl()) {

  WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];

  AppSchemeHandler *schemeHandler = [[AppSchemeHandler alloc] init];
  schemeHandler.rootPath = PagesRootPath();
  [config setURLSchemeHandler:schemeHandler forURLScheme:@"app"];

  WKUserContentController *userContentController =
      [[WKUserContentController alloc] init];
  ScriptMessageHandler *scriptHandler = [[ScriptMessageHandler alloc] init];
  scriptHandler.onMessage = ^(id body) {
    if (!impl->messageReceiver)
      return;
    NSString *nsMessage = nil;
    if ([body isKindOfClass:[NSString class]]) {
      nsMessage = (NSString *)body;
    } else if (body) {
      if ([NSJSONSerialization isValidJSONObject:body]) {
        NSData *data = [NSJSONSerialization dataWithJSONObject:body
                                                       options:0
                                                         error:nil];
        if (data) {
          nsMessage = [[NSString alloc] initWithData:data
                                            encoding:NSUTF8StringEncoding];
        }
      }
    }
    if (nsMessage == nil) {
      return;
    }
    // printf("MacWebView::messageReceiver: %s\n", nsMessage.UTF8String);
    std::string messageCppStr = [nsMessage UTF8String];
    impl->messageReceiver(messageCppStr);
  };
  impl->scriptHandler = scriptHandler;
  [userContentController addScriptMessageHandler:scriptHandler
                                            name:@"pluginEditor"];
  config.userContentController = userContentController;

  PluginWKWebView *webView =
      [[PluginWKWebView alloc] initWithFrame:CGRectMake(0, 0, 0, 0)
                               configuration:config];  
  impl->webView = webView;

  if (@available(macOS 13.3, *)) {
    if ([webView respondsToSelector:@selector(setInspectable:)]) {
      [webView setInspectable:true];
    }
  }
}

MacWebView::~MacWebView() {
  if (impl->webView) {
    WKUserContentController *uc =
        impl->webView.configuration.userContentController;
    [uc removeScriptMessageHandlerForName:@"pluginEditor"];
  }
  if (impl->scriptHandler) {
    ((ScriptMessageHandler *)impl->scriptHandler).onMessage = nil;
    impl->scriptHandler = nil;
  }
}

void MacWebView::attachToParent(void *parent) {
  // printf("MacWebView::attachToParent\n");
#if TARGET_OS_IPHONE
  UIView *parentView = (__bridge UIView *)parent;
#else
  NSView *parentView = (__bridge NSView *)parent;
#endif
  if (!parentView || !impl->webView) {
    return;
  }
  [parentView addSubview:impl->webView];
  [impl->webView setFrame:parentView.bounds];
#if TARGET_OS_IPHONE
  impl->webView.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  //remove top and bottom insets
  impl->webView.scrollView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
#else
  impl->webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
#endif
}

void MacWebView::removeFromParent() {
  if (!impl->webView) {
    return;
  }
  [impl->webView removeFromSuperview];
}

void MacWebView::setFrame(int x, int y, int width, int height) {
  // printf("MacWebView::setFrame: %d, %d, %d, %d\n", x, y, width, height);
  if (!impl->webView) {
    return;
  }
#if TARGET_OS_IPHONE
  CGRect frame = CGRectMake(x, y, width, height);
#else
  NSRect frame = NSMakeRect(x, y, width, height);
#endif
  [impl->webView setFrame:frame];
}

void MacWebView::loadUrl(const std::string &urlCppStr) {
  // printf("MacWebView::loadUrl: %s\n", urlCppStr.c_str());
  if (!impl->webView) {
    return;
  }
  NSString *nsUrl = [NSString stringWithUTF8String:urlCppStr.c_str()];
  NSURL *url = [NSURL URLWithString:nsUrl];
  if (!url) {
    return;
  }
  [impl->webView loadRequest:[NSURLRequest requestWithURL:url]];
}

void MacWebView::sendMessage(const std::string &message) {
  if (!impl->webView) {
    return;
  }
  NSString *nsMessage = [NSString stringWithUTF8String:message.c_str()];
  // clang-format off
  NSString *jsCode = [NSString
      // stringWithFormat:@"window.dispatchEvent(new CustomEvent('native-message', {detail: %@}))",
      stringWithFormat:@"window.pluginEditorCallback && window.pluginEditorCallback(%@)",
                       nsMessage];
  // clang-format on
  // printf("MacWebView::sendMessage: %s\n", jsCode.UTF8String);
  dispatch_async(dispatch_get_main_queue(), ^{
    [impl->webView evaluateJavaScript:jsCode completionHandler:nil];
  });
}

void MacWebView::setMessageReceiver(
    std::function<void(const std::string &)> receiver) {
  impl->messageReceiver = receiver;
}
