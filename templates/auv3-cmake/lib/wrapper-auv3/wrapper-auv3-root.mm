#import "./wrapper-auv3-root.h"
#include "../common/parameter_builder_impl.h"
#include "../common/synthesizer_base.h"
#include <AudioToolbox/AudioToolbox.h>
#include <cstdlib>
#include <memory>

@implementation WrapperAuv3AudioUnit {
  AUAudioUnitBus *_outputBus;
  AUAudioUnitBusArray *_outputBusArray;
  AUAudioUnitBusArray *_inputBusArray;
  AUAudioFrameCount mMaxFramesToRender;

  std::unique_ptr<SynthesizerBase> synth;
}

static AUParameter *
createAUParameterFromItem(const sonic_common::ParameterItem &entry) {
  AudioUnitParameterOptions paramOptions =
      kAudioUnitParameterFlag_IsWritable | kAudioUnitParameterFlag_IsReadable;
  if (entry.type == sonic_common::ParameterType::Enum) {
    paramOptions |= kAudioUnitParameterFlag_ValuesHaveStrings;
  }
  AUParameter *param = [AUParameterTree
      createParameterWithIdentifier:[NSString
                                        stringWithUTF8String:entry.identifier
                                                                 .c_str()]
                               name:[NSString stringWithUTF8String:entry.label
                                                                       .c_str()]
                            address:entry.address
                                min:(float)entry.minValue
                                max:(float)entry.maxValue
                               unit:kAudioUnitParameterUnit_Generic
                           unitName:nil
                              flags:paramOptions
                       valueStrings:nil
                dependentParameters:nil];
  param.value = (float)entry.defaultValue;
  return param;
}

- (instancetype)
    initWithComponentDescription:(AudioComponentDescription)componentDescription
                         options:(AudioComponentInstantiationOptions)options
                           error:(NSError **)outError {
  printf("initWithComponentDescription 0233\n");
  self = [super initWithComponentDescription:componentDescription
                                     options:options
                                       error:outError];
  if (!self) {
    return nil;
  }

  synth = std::unique_ptr<SynthesizerBase>(createSynthesizerInstance());
  auto parameterBuilder = sonic_common::ParameterBuilderImpl();
  synth->setupParameters(parameterBuilder);
  auto parameterItems = parameterBuilder.getItems();

  NSMutableArray *auParams = [NSMutableArray array];
  for (const auto &entry : parameterItems) {
    [auParams addObject:createAUParameterFromItem(entry)];
  }
  self.parameterTree = [AUParameterTree createTreeWithChildren:auParams];

  AVAudioFormat *format =
      [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0
                                                     channels:2];
  _outputBus = [[AUAudioUnitBus alloc] initWithFormat:format error:outError];
  if (!_outputBus) {
    return nil;
  }
  _outputBus.maximumChannelCount = 2;

  _outputBusArray =
      [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                             busType:AUAudioUnitBusTypeOutput
                                              busses:@[ _outputBus ]];
  _inputBusArray =
      [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                             busType:AUAudioUnitBusTypeInput
                                              busses:@[]];

  mMaxFramesToRender = 1024;

  return self;
}

- (AUAudioUnitBusArray *)outputBusses {
  return _outputBusArray;
}

- (AUAudioUnitBusArray *)inputBusses {
  return _inputBusArray;
}

- (AUAudioFrameCount)getMaximumFramesToRender {
  return mMaxFramesToRender;
}

- (void)setMaximumFramesToRender:(AUAudioFrameCount)maximumFramesToRender {
  mMaxFramesToRender = maximumFramesToRender;
}

- (BOOL)allocateRenderResourcesAndReturnError:(NSError *_Nullable *)outError {
  auto sampleRate = self.outputBusses[0].format.sampleRate;
  auto maxFrameLength = mMaxFramesToRender;
  printf("call prepareProcessing, sampleRate: %f, maxFrameLength: %u\n",
         sampleRate, maxFrameLength);
  synth->prepareProcessing(sampleRate, maxFrameLength);
  return [super allocateRenderResourcesAndReturnError:outError];
}

- (AUInternalRenderBlock)internalRenderBlock {
  return [^AUAudioUnitStatus(
      AudioUnitRenderActionFlags *actionFlags, const AudioTimeStamp *timestamp,
      AUAudioFrameCount frameCount, NSInteger outputBusNumber,
      AudioBufferList *outputData, const AURenderEvent *realtimeEventListHead,
      AURenderPullInputBlock pullInputBlock) {
    (void)actionFlags;
    (void)timestamp;
    (void)outputBusNumber;
    (void)realtimeEventListHead;
    (void)pullInputBlock;

    float *left = (float *)outputData->mBuffers[0].mData;
    float *right = (float *)outputData->mBuffers[1].mData;
    synth->processAudio(left, right, frameCount);

    return noErr;
  } copy];
}

@end
