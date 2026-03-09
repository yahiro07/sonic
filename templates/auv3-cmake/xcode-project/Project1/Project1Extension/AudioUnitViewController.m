#import "./AudioUnitViewController.h"
#import <AppKit/AppKit.h>

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

// - (void)connectUIToAudioUnitIfPossible {
//   if (self.audioUnit == nil || self.viewFrame == nil) {
//     return;
//   }

//   // self.embeddedViewController.audioUnit = self.audioUnit;
// }

// - (void)loadView {
//   self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 480, 240)];
// }

// - (void)ensureEmbeddedViewController {
// if (self.embeddedViewController != nil) {
//   return;
// }
// if (self.viewFrame != nil) {
//   return;
// }

// WrapperAuv3ViewController *viewController =
//     [[WrapperAuv3ViewController alloc] init];
// self.embeddedViewController = viewController;

// [self addChildViewController:viewController];
// viewController.view.translatesAutoresizingMaskIntoConstraints = NO;
// [self.view addSubview:viewController.view];

// [NSLayoutConstraint activateConstraints:@[
//   [viewController.view.leadingAnchor
//       constraintEqualToAnchor:self.view.leadingAnchor],
//   [viewController.view.trailingAnchor
//       constraintEqualToAnchor:self.view.trailingAnchor],
//   [viewController.view.topAnchor
//   constraintEqualToAnchor:self.view.topAnchor],
//   [viewController.view.bottomAnchor
//       constraintEqualToAnchor:self.view.bottomAnchor],
// ]];
// }

@end
