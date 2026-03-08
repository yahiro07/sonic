#import "./AudioUnitFactory.h"

// #import <Foundation/Foundation.h>

// #import "../../../lib/wrapper-auv3/wrapper-auv3-root.h"

// @interface AudioUnitFactory ()
// @property(nonatomic, strong, nullable) AUAudioUnit *auAudioUnit;
// @end

@implementation AudioUnitFactory

- (void)beginRequestWithExtensionContext:(NSExtensionContext *)context {
  (void)context;
}

- (nullable AUAudioUnit *)
    createAudioUnitWithComponentDescription:
        (AudioComponentDescription)componentDescription
                                      error:(NSError *_Nullable *_Nullable)
                                                outError {
  return [[WrapperAuv3AudioUnit alloc]
      initWithComponentDescription:componentDescription
                           options:0
                             error:outError];
}

@end
