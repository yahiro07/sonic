#pragma once

#import <AVFoundation/AVFoundation.h>
#import <CoreAudioKit/AUViewController.h>

@interface WrapperAuv3AudioUnit : AUAudioUnit
@end

// @interface WrapperAuv3ViewController : AUViewController
// @property(nonatomic, strong) WrapperAuv3AudioUnit *audioUnit;
// @end

@interface WrapperAuv3ViewFrame : NSObject
- (void)connectViewToAudioUnit:(WrapperAuv3AudioUnit *)audioUnit
                viewController:(AUViewController *)viewController;
- (void)disconnectViewFromAudioUnit;
@end