#import "./AudioUnitViewController.h"
#include <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

@interface AudioUnitViewController ()
@property(nonatomic, strong, nullable) WrapperAuv3AudioUnit *audioUnit;
@property(nonatomic, strong, nullable) WrapperAuv3ViewFrame *viewFrame;
@end

@implementation AudioUnitViewController

- (nullable AUAudioUnit *)
    createAudioUnitWithComponentDescription:
        (AudioComponentDescription)componentDescription
                                      error:(NSError *_Nullable *_Nullable)
                                                outError {
  printf(
      "AudioUnitViewController createAudioUnitWithComponentDescription 0940\n");
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
  printf("AudioUnitViewController loadView\n");
  self.view = [[NSView alloc]
      initWithFrame:NSMakeRect(0, 0, 480, 240)]; // temporal size
  self.view.wantsLayer = YES;
  self.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
}

- (void)viewDidLoad {
  printf("AudioUnitViewController viewDidLoad\n");
  [super viewDidLoad];
  [self ensureInitialized];
}

#pragma mark -

- (void)ensureInitialized {
  if (self.audioUnit && !self.viewFrame) {
    printf("AudioUnitViewController initialize\n");
    self.viewFrame = [[WrapperAuv3ViewFrame alloc] init];
    [self.viewFrame connectViewToAudioUnit:self.audioUnit viewController:self];
  }
}

- (void)dealloc {
  if (self.viewFrame) {
    [self.viewFrame disconnectViewFromAudioUnit];
    self.viewFrame = nil;
  }
  self.audioUnit = nil;
}

@end
