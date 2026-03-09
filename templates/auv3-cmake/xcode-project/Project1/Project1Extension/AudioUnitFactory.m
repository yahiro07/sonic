#import "./AudioUnitFactory.h"
#import <AppKit/AppKit.h>

@interface AudioUnitFactory ()
@property(nonatomic, strong, nullable) WrapperAuv3AudioUnit *audioUnit;
@property(nonatomic, strong, nullable)
    WrapperAuv3ViewController *embeddedViewController;
@end

@implementation AudioUnitFactory

- (void)loadView {
  self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 480, 240)];
}

- (void)beginRequestWithExtensionContext:(NSExtensionContext *)context {
  (void)context;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.preferredContentSize = NSMakeSize(480, 240);
  [self ensureEmbeddedViewController];
  [self connectUIToAudioUnitIfPossible];
}

- (nullable AUAudioUnit *)
    createAudioUnitWithComponentDescription:
        (AudioComponentDescription)componentDescription
                                      error:(NSError *_Nullable *_Nullable)
                                                outError {
  printf("AudioUnitFactory createAudioUnitWithComponentDescription 0940\n");
  self.audioUnit = [[WrapperAuv3AudioUnit alloc]
      initWithComponentDescription:componentDescription
                           options:0
                             error:outError];
  [self connectUIToAudioUnitIfPossible];
  return self.audioUnit;
}

- (void)ensureEmbeddedViewController {
  if (self.embeddedViewController != nil) {
    return;
  }

  WrapperAuv3ViewController *viewController =
      [[WrapperAuv3ViewController alloc] init];
  self.embeddedViewController = viewController;

  [self addChildViewController:viewController];
  viewController.view.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:viewController.view];

  [NSLayoutConstraint activateConstraints:@[
    [viewController.view.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [viewController.view.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    [viewController.view.topAnchor constraintEqualToAnchor:self.view.topAnchor],
    [viewController.view.bottomAnchor
        constraintEqualToAnchor:self.view.bottomAnchor],
  ]];
}

- (void)connectUIToAudioUnitIfPossible {
  if (self.audioUnit == nil || self.embeddedViewController == nil) {
    return;
  }

  self.embeddedViewController.audioUnit = self.audioUnit;
}

@end
