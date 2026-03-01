#include "app_window_mac.h"
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include <stdio.h>

@interface AppWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation AppWindowDelegate
- (BOOL)windowShouldClose:(NSWindow *)sender {
  [NSApp terminate:nil];
  return YES;
}
@end

AppWindowMac::AppWindowMac() = default;

AppWindowMac::~AppWindowMac() {
  @autoreleasepool {
    if (delegate_) {
      AppWindowDelegate *delegate = (AppWindowDelegate *)delegate_;
      [delegate release];
    }
    if (window_) {
      NSWindow *window = (NSWindow *)window_;
      [window close];
    }
  }
}

void AppWindowMac::show() {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    if (!delegate_) {
      delegate_ = [[AppWindowDelegate alloc] init];
    }

    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSWindow *window =
        [[NSWindow alloc] initWithContentRect:frame
                                    styleMask:(NSWindowStyleMaskTitled |
                                               NSWindowStyleMaskClosable |
                                               NSWindowStyleMaskResizable)
                                      backing:NSBackingStoreBuffered
                                        defer:NO];

    [window setTitle:@"VST Dev Host"];
    [window center];
    NSView *contentView = [[NSView alloc]
        initWithFrame:NSMakeRect(0, 0, frame.size.width, frame.size.height)];
    contentView.wantsLayer = YES;
    contentView.layer.backgroundColor =
        [[NSColor windowBackgroundColor] CGColor];

    NSTextField *label = [[NSTextField alloc]
        initWithFrame:NSMakeRect((frame.size.width - 240) / 2,
                                 (frame.size.height - 40) / 2, 240, 40)];
    label.stringValue = @"Hello World";
    label.alignment = NSTextAlignmentCenter;
    label.editable = NO;
    label.bezeled = NO;
    label.drawsBackground = NO;
    label.font = [NSFont boldSystemFontOfSize:24];
    label.translatesAutoresizingMaskIntoConstraints = YES;
    [contentView addSubview:label];

    window.contentView = contentView;
    window.delegate = (id<NSWindowDelegate>)delegate_;
    [window makeKeyAndOrderFront:nil];

    [NSApp activateIgnoringOtherApps:YES];

    window_ = window;
    printf("Window should be visible now.\n");
  }
}

void AppWindowMac::loop() {
  @autoreleasepool {
    [NSApp run];
  }
}
