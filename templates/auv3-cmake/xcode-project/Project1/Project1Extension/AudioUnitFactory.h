#pragma once

#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface AudioUnitFactory
    : NSObject <AUAudioUnitFactory, NSExtensionRequestHandling>
@end

NS_ASSUME_NONNULL_END
