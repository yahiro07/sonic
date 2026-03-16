#pragma once

#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudioKit/AUViewController.h>
#import <Foundation/Foundation.h>

#import "../../../framework/platform/auv3/wrapper-auv3-root.h"

// Keep the principal AUv3 UI entry point in the extension target and delegate
// the actual AU/UI implementation to the static library.

@interface AudioUnitViewController : AUViewController <AUAudioUnitFactory>
@end
