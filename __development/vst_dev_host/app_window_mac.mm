#include "app_window_mac.h"
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include <stdio.h>

@interface AppWindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) void (*selectionCallback)(const std::string &);
- (void)midiDeviceSelected:(NSMenuItem *)sender;
@end

@implementation AppWindowDelegate
- (BOOL)windowShouldClose:(NSWindow *)sender {
  [NSApp terminate:nil];
  return YES;
}

- (void)midiDeviceSelected:(NSMenuItem *)sender {
  if (self.selectionCallback) {
    self.selectionCallback([sender.representedObject UTF8String]);
  }
}
@end

struct AppWindowMac::InternalStates {
  void *window{nullptr};
  void *delegate{nullptr};
  void (*selection_callback)(const std::string &){nullptr};
};

AppWindowMac::AppWindowMac() : states(std::make_unique<InternalStates>()) {}

AppWindowMac::~AppWindowMac() {
  @autoreleasepool {
    if (states->delegate) {
      AppWindowDelegate *delegate = (AppWindowDelegate *)states->delegate;
      [delegate release];
    }
    if (states->window) {
      NSWindow *window = (NSWindow *)states->window;
      [window close];
    }
  }
}

void AppWindowMac::show() {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    if (!states->delegate) {
      states->delegate = [[AppWindowDelegate alloc] init];
    }

    // Set callback if it was already subscribed
    ((AppWindowDelegate *)states->delegate).selectionCallback =
        states->selection_callback;

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
    window.delegate = (id<NSWindowDelegate>)states->delegate;
    [window makeKeyAndOrderFront:nil];

    [NSApp activateIgnoringOtherApps:YES];

    states->window = window;
    printf("Window should be visible now.\n");
  }
}

void AppWindowMac::loop() {
  @autoreleasepool {
    [NSApp run];
  }
}

void AppWindowMac::refreshMidiInputDeviceListMenu(
    const std::vector<MidiDeviceInfo> &devices,
    const std::string &selectedDeviceKey) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    NSMenu *mainMenu = [NSApp mainMenu];
    if (!mainMenu) {
      mainMenu = [[NSMenu alloc] init];
      [NSApp setMainMenu:mainMenu];

      // macOS requires the first menu to be the "Application" menu
      NSMenuItem *appMenuItem = [[NSMenuItem alloc] initWithTitle:@""
                                                           action:nil
                                                    keyEquivalent:@""];
      [mainMenu addItem:appMenuItem];
      NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@""];
      appMenuItem.submenu = appMenu;

      [appMenu
          addItemWithTitle:[NSString
                               stringWithFormat:@"Quit %@",
                                                [[NSProcessInfo processInfo]
                                                    processName]]
                    action:@selector(terminate:)
             keyEquivalent:@"q"];
    }

    NSString *menuName = @"MIDI Input";
    NSMenuItem *midiMenuItem = [mainMenu itemWithTitle:menuName];
    if (!midiMenuItem) {
      midiMenuItem = [[NSMenuItem alloc] initWithTitle:menuName
                                                action:nil
                                         keyEquivalent:@""];
      [mainMenu addItem:midiMenuItem];
    }

    NSMenu *midiMenu = [[NSMenu alloc] initWithTitle:menuName];
    midiMenuItem.submenu = midiMenu;

    for (const auto &device : devices) {
      NSMenuItem *item = [[NSMenuItem alloc]
          initWithTitle:[NSString
                            stringWithUTF8String:device.displayName.c_str()]
                 action:@selector(midiDeviceSelected:)
          keyEquivalent:@""];
      item.target = (id)states->delegate;
      item.representedObject =
          [NSString stringWithUTF8String:device.deviceKey.c_str()];
      if (device.deviceKey == selectedDeviceKey) {
        [item setState:NSControlStateValueOn];
      }
      [midiMenu addItem:item];
    }
  }
}

void AppWindowMac::subscribeMidiInputDeviceSelection(
    void (*callback)(const std::string &deviceKey)) {
  states->selection_callback = callback;
  if (states->delegate) {
    AppWindowDelegate *delegate = (AppWindowDelegate *)states->delegate;
    delegate.selectionCallback = callback;
  }
}

void AppWindowMac::clearMenuSubscriptions() {
  states->selection_callback = nullptr;
  if (states->delegate) {
    AppWindowDelegate *delegate = (AppWindowDelegate *)states->delegate;
    delegate.selectionCallback = nullptr;
  }
}
