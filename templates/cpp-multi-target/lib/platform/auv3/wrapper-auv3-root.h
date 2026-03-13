#pragma once

#import <AVFoundation/AVFoundation.h>
#import <CoreAudioKit/AUViewController.h>

@interface WrapperAuv3AudioUnit : AUAudioUnit
// - (void)getDesiredEditorSize:(uint32_t *)width height:(uint32_t *)height;
@end

@interface WrapperAuv3ViewFrame : NSObject
- (void)connectViewToAudioUnit:(WrapperAuv3AudioUnit *)audioUnit
                viewController:(AUViewController *)viewController;
- (void)disconnectViewFromAudioUnit;
@end