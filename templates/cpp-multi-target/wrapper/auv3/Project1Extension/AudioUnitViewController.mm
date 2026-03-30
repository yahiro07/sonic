#import "./AudioUnitViewController.h"
#include "wrapper-auv3-root.h"
#include <Foundation/Foundation.h>

#if !__has_feature(objc_arc)
#error "this code requires ARC. Enable -fobjc-arc for Objective-C sources."
#endif

@interface AudioUnitViewController ()
@property WrapperAuv3AudioUnit *audioUnit;
@property WrapperAuv3ViewFrame *viewFrame;
@end

@implementation AudioUnitViewController

- (nullable AUAudioUnit *)
    createAudioUnitWithComponentDescription:
        (AudioComponentDescription)componentDescription
                                      error:(NSError *_Nullable *_Nullable)
                                                outError {

  [LoggerWrapper start];
  [LoggerWrapper trace:@"----------------------------------------"];
  [LoggerWrapper
      trace:@"AudioUnitViewController createAudioUnitWithComponentDescription"];
  auto bundleId = [[NSBundle mainBundle] bundleIdentifier];
  auto bundlePath = [[NSBundle mainBundle] bundlePath];
  [LoggerWrapper
      log:[NSString stringWithFormat:@"Bundle ID: %s", [bundleId UTF8String]]];
  [LoggerWrapper log:[NSString stringWithFormat:@"Loaded From: %s",
                                                [bundlePath UTF8String]]];

  WrapperAuv3AudioUnit *audioUnit = [[WrapperAuv3AudioUnit alloc]
      initWithComponentDescription:componentDescription
                           options:0
                             error:outError];
  self.audioUnit = audioUnit;
  dispatch_async(dispatch_get_main_queue(), ^{
    [self ensureInitialized];
  });
  return audioUnit;
}

- (void)loadView {
  // printf("AudioUnitViewController loadView\n");
  self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 480, 240)];
  self.view.wantsLayer = YES;
  self.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
}

- (void)viewDidLoad {
  // printf("AudioUnitViewController viewDidLoad\n");
  [super viewDidLoad];
  [self ensureInitialized];
}

#pragma mark -

- (void)ensureInitialized {
  if (self.audioUnit && !self.viewFrame) {
    // printf("AudioUnitViewController initialize\n");
    self.viewFrame = [[WrapperAuv3ViewFrame alloc] init];
    [self.viewFrame connectViewToAudioUnit:self.audioUnit viewController:self];
  }
}

- (void)dealloc {
  if (self.viewFrame) {
    [self.viewFrame disconnectViewFromAudioUnit];
  }
  [LoggerWrapper stop];
}

@end
