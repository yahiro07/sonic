#include "views.h"
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

void showWindow() {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    static AppWindowDelegate *windowDelegate = nil;
    if (!windowDelegate) {
      windowDelegate = [[AppWindowDelegate alloc] init];
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
    window.delegate = windowDelegate;
    [window makeKeyAndOrderFront:nil];

    [NSApp activateIgnoringOtherApps:YES];

    printf("Window should be visible now.\n");
  }
}

void windowLoop() {
  @autoreleasepool {
    [NSApp run];
  }
}
