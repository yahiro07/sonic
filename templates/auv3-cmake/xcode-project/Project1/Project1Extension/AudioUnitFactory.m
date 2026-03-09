#import "./AudioUnitFactory.h"
#import <AppKit/AppKit.h>

@interface AudioUnitFactory ()
@property(nonatomic, strong, nullable) WrapperAuv3AudioUnit *audioUnit;
@property(nonatomic, strong, nullable) WrapperAuv3ViewFrame *viewFrame;
@end

@implementation AudioUnitFactory

- (void)beginRequestWithExtensionContext:(NSExtensionContext *)context {
  (void)context;
}

- (nullable AUAudioUnit *)
    createAudioUnitWithComponentDescription:
        (AudioComponentDescription)componentDescription
                                      error:(NSError *_Nullable *_Nullable)
                                                outError {
  printf("AudioUnitFactory createAudioUnitWithComponentDescription 0940\n");
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
  self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 480, 240)];
  self.preferredContentSize = NSMakeSize(480, 240);
}

- (void)viewDidLoad {
  printf("AudioUnitFactory viewDidLoad\n");
  [super viewDidLoad];
  [self ensureInitialized];
}

#pragma mark -

- (void)ensureInitialized {
  if (self.audioUnit && !self.viewFrame) {
    printf("AudioUnitFactory initialize\n");
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
