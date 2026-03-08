#pragma once

#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>

#import "../../../lib/wrapper-auv3/wrapper-auv3-root.h"

// @interface AudioUnitFactory
//     : NSObject <AUAudioUnitFactory, NSExtensionRequestHandling>
// @end

@interface WrapperAuv3ViewController (AUAudioUnitFactory) <AUAudioUnitFactory>
@end