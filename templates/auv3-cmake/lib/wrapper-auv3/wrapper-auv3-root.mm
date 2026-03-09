#import "./wrapper-auv3-root.h"
#include <CoreAudioKit/CoreAudioKit.h>
#include <objc/NSObject.h>
// #include "../common/parameter_builder_impl.h"
#include "../common/synthesizer_base.h"
#include <AudioToolbox/AudioToolbox.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

@interface WrapperAuv3AudioUnit () {
  AUAudioUnitBus *_outputBus;
  AUAudioUnitBusArray *_outputBusArray;
  AUAudioUnitBusArray *_inputBusArray;
  std::unique_ptr<SynthesizerBase> synth;
}
@end

@implementation WrapperAuv3AudioUnit

// static AUParameter *
// createAUParameterFromItem(const sonic_common::ParameterItem &entry) {
//   AudioUnitParameterOptions paramOptions =
//       kAudioUnitParameterFlag_IsWritable |
//       kAudioUnitParameterFlag_IsReadable;
//   if (entry.type == sonic_common::ParameterType::Enum) {
//     paramOptions |= kAudioUnitParameterFlag_ValuesHaveStrings;
//   }
//   AUParameter *param = [AUParameterTree
//       createParameterWithIdentifier:[NSString
//                                         stringWithUTF8String:entry.identifier
//                                                                  .c_str()]
//                                name:[NSString
//                                stringWithUTF8String:entry.label
//                                                                        .c_str()]
//                             address:entry.address
//                                 min:(float)entry.minValue
//                                 max:(float)entry.maxValue
//                                unit:kAudioUnitParameterUnit_Generic
//                            unitName:nil
//                               flags:paramOptions
//                        valueStrings:nil
//                 dependentParameters:nil];
//   param.value = (float)entry.defaultValue;
//   return param;
// }

- (instancetype)
    initWithComponentDescription:(AudioComponentDescription)componentDescription
                         options:(AudioComponentInstantiationOptions)options
                           error:(NSError **)outError {
  printf("initWithComponentDescription 0930\n");
  self = [super initWithComponentDescription:componentDescription
                                     options:options
                                       error:outError];
  if (!self) {
    return nil;
  }

  synth = std::unique_ptr<SynthesizerBase>(createSynthesizerInstance());
  // auto parameterBuilder = sonic_common::ParameterBuilderImpl();
  // synth->setupParameters(parameterBuilder);
  // auto parameterItems = parameterBuilder.getItems();

  // NSMutableArray *auParams = [NSMutableArray array];
  // for (const auto &entry : parameterItems) {
  //   [auParams addObject:createAUParameterFromItem(entry)];
  // }
  // self.parameterTree = [AUParameterTree createTreeWithChildren:auParams];

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

  return self;
}

- (void)getDesiredEditorSize:(uint32_t *)width height:(uint32_t *)height {
  if (synth) {
    synth->getDesiredEditorSize(*width, *height);
  }
}

- (AUAudioUnitBusArray *)outputBusses {
  return _outputBusArray;
}

- (AUAudioUnitBusArray *)inputBusses {
  return _inputBusArray;
}

- (BOOL)allocateRenderResourcesAndReturnError:(NSError *_Nullable *)outError {
  auto sampleRate = self.outputBusses[0].format.sampleRate;
  auto maxFrameLength = self.maximumFramesToRender;
  printf("call prepareProcessing, sampleRate: %f, maxFrameLength: %u\n",
         sampleRate, maxFrameLength);
  // synth->prepareProcessing(sampleRate, maxFrameLength);
  return [super allocateRenderResourcesAndReturnError:outError];
}

static void debugFillNoise(float *bufferL, float *bufferR, uint32_t frames) {
  for (uint32_t i = 0; i < frames; i++) {
    float y = (((float)rand() / RAND_MAX) * 2.f - 1.f) * .1f;
    bufferL[i] = y;
    bufferR[i] = y;
  }
}

- (AUInternalRenderBlock)internalRenderBlock {
  // SynthesizerBase *synthPtr = synth.get();
  AUAudioFrameCount maxFramesToRender = self.maximumFramesToRender;

  AUInternalRenderBlock block = ^AUAudioUnitStatus(
      AudioUnitRenderActionFlags *actionFlags, const AudioTimeStamp *timestamp,
      AUAudioFrameCount frameCount, NSInteger outputBusNumber,
      AudioBufferList *outputData, const AURenderEvent *realtimeEventListHead,
      AURenderPullInputBlock pullInputBlock) {
    (void)actionFlags;
    (void)timestamp;
    (void)outputBusNumber;
    (void)pullInputBlock;

    // Handle MIDI and Parameter events
    // const AURenderEvent *event = realtimeEventListHead;
    // while (event != nullptr) {
    //   if (event->head.eventType == AURenderEventMIDI) {
    //     uint8_t status = event->MIDI.data[0] & 0xF0;
    //     uint8_t data1 = event->MIDI.data[1];
    //     uint8_t data2 = event->MIDI.data[2];

    //     if (status == 0x90 && data2 > 0) {
    //       synthPtr->noteOn(data1, (double)data2 / 127.0);
    //     } else if (status == 0x80 || (status == 0x90 && data2 == 0)) {
    //       synthPtr->noteOff(data1);
    //     }
    //   } else if (event->head.eventType == AURenderEventParameter) {
    //     synthPtr->setParameter(event->parameter.parameterAddress,
    //                            event->parameter.value);
    //   }
    //   event = event->head.next;
    // }

    // // process audio
    // if (outputData == nullptr || outputData->mNumberBuffers == 0) {
    //   return noErr;
    // }

    // auto clearOutput = [&]() {
    //   if (outputData == nullptr) {
    //     return;
    //   }
    //   for (UInt32 i = 0; i < outputData->mNumberBuffers; i++) {
    //     AudioBuffer *b = &outputData->mBuffers[i];
    //     if (b->mData != nullptr && b->mDataByteSize > 0) {
    //       std::memset(b->mData, 0, (size_t)b->mDataByteSize);
    //     }
    //   }
    //   if (actionFlags != nullptr) {
    //     *actionFlags |= kAudioUnitRenderAction_OutputIsSilence;
    //   }
    // };

    // if (frameCount > maxFramesToRender) {
    //   clearOutput();
    //   return kAudioUnitErr_TooManyFramesToProcess;
    // }

    // Many hosts will provide either:
    // - non-interleaved stereo: mNumberBuffers == 2 (L/R)
    // - interleaved stereo: mNumberBuffers == 1 (LRLR...)
    // if (outputData->mNumberBuffers >= 2) {
    //   AudioBuffer *bufferL = &outputData->mBuffers[0];
    //   AudioBuffer *bufferR = &outputData->mBuffers[1];
    //   if (bufferL->mData == nullptr || bufferR->mData == nullptr) {
    //     clearOutput();
    //     return noErr;
    //   }

    //   auto neededBytes = (UInt32)(frameCount * sizeof(float));
    //   if (bufferL->mDataByteSize < neededBytes ||
    //       bufferR->mDataByteSize < neededBytes) {
    //     clearOutput();
    //     return noErr;
    //   }

    //   float *left = (float *)bufferL->mData;
    //   float *right = (float *)bufferR->mData;
    //   // debugFillNoise(left, right, frameCount);
    //   synthPtr->processAudio(left, right, frameCount);
    // } else {
    //   // interleaved
    //   AudioBuffer *buffer = &outputData->mBuffers[0];
    //   if (buffer->mData == nullptr) {
    //     clearOutput();
    //     return noErr;
    //   }

    //   UInt32 channels = buffer->mNumberChannels;
    //   if (channels == 0) {
    //     clearOutput();
    //     return noErr;
    //   }

    //   auto neededSamples = (size_t)frameCount * (size_t)channels;
    //   auto availableSamples = (size_t)(buffer->mDataByteSize /
    //   sizeof(float)); if (availableSamples < neededSamples) {
    //     clearOutput();
    //     return noErr;
    //   }

    //   static thread_local std::vector<float> tmpL;
    //   static thread_local std::vector<float> tmpR;
    //   if (tmpL.size() < (size_t)frameCount) {
    //     tmpL.resize((size_t)frameCount);
    //     tmpR.resize((size_t)frameCount);
    //   }

    //   float *interleaved = (float *)buffer->mData;

    //   if (channels == 1) {
    //     for (AUAudioFrameCount i = 0; i < frameCount; i++) {
    //       tmpL[(size_t)i] = interleaved[(size_t)i];
    //     }
    //     synthPtr->processAudio(tmpL.data(), tmpL.data(), frameCount);
    //     for (AUAudioFrameCount i = 0; i < frameCount; i++) {
    //       interleaved[(size_t)i] = tmpL[(size_t)i];
    //     }
    //   } else {
    //     // Use the first two channels as L/R; preserve any additional
    //     channels. for (AUAudioFrameCount i = 0; i < frameCount; i++) {
    //       size_t base = (size_t)i * (size_t)channels;
    //       tmpL[(size_t)i] = interleaved[base + 0];
    //       tmpR[(size_t)i] = interleaved[base + 1];
    //     }

    //     synthPtr->processAudio(tmpL.data(), tmpR.data(), frameCount);

    //     for (AUAudioFrameCount i = 0; i < frameCount; i++) {
    //       size_t base = (size_t)i * (size_t)channels;
    //       interleaved[base + 0] = tmpL[(size_t)i];
    //       interleaved[base + 1] = tmpR[(size_t)i];
    //     }
    //   }
    // }

    return noErr;
  };

  return [block copy];
}

@end

//------------------------------------------------------------

@interface WrapperAuv3ViewFrame () {
  NSTextField *_label;
}
@end

@implementation WrapperAuv3ViewFrame

- (void)connectViewToAudioUnit:(WrapperAuv3AudioUnit *)audioUnit
                viewController:(AUViewController *)viewController {
  printf("WrapperAuv3ViewFrame connectViewToAudioUnit\n");

  NSView *root = viewController.view;
  root.wantsLayer = YES;
  root.layer.backgroundColor = [[NSColor colorWithCalibratedWhite:0.1
                                                            alpha:1.0] CGColor];

  _label =
      [NSTextField labelWithString:@"Hello from Wrapper AUv3 static library"];
  _label.translatesAutoresizingMaskIntoConstraints = NO;
  _label.font = [NSFont systemFontOfSize:20 weight:NSFontWeightSemibold];
  _label.textColor = [NSColor whiteColor];

  [root addSubview:_label];

  [NSLayoutConstraint activateConstraints:@[
    [_label.centerXAnchor constraintEqualToAnchor:root.centerXAnchor],
    [_label.centerYAnchor constraintEqualToAnchor:root.centerYAnchor],
  ]];

  uint32_t width = 0;
  uint32_t height = 0;
  [audioUnit getDesiredEditorSize:&width height:&height];
  if (width && height) {
    viewController.preferredContentSize = NSMakeSize(width, height);
  }
}

- (void)disconnectViewFromAudioUnit {
  printf("WrapperAuv3ViewFrame disconnectViewFromAudioUnit\n");
  _label = nil;
}

@end

//------------------------------------------------------------

// @interface WrapperAuv3ViewController () {

//   WrapperAuv3ViewFrame *_viewFrame;
// }
// @end

// @implementation WrapperAuv3ViewController

// - (void)loadView {
//   self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 480, 240)];
// }

// - (void)viewDidLoad {
//   printf("WrapperAuv3ViewController viewDidLoad\n");
//   [super viewDidLoad];
//   [self setupUiView];
//   [self refreshUiState];
// }

// - (void)dealloc {
//   [self cleanupUiView];
// }

// - (WrapperAuv3AudioUnit *)getAudioUnit {
//   return _audioUnit;
// }
// - (void)setAudioUnit:(WrapperAuv3AudioUnit *)audioUnit {
//   printf("WrapperAuv3ViewController setAudioUnit\n");
//   _audioUnit = audioUnit;
//   dispatch_async(dispatch_get_main_queue(), ^{
//     if ([self isViewLoaded]) {
//       [self refreshUiState];
//     }
//   });
// }
// #pragma mark -

// - (void)setupUiView {
//   printf("WrapperAuv3ViewController setupUiView\n");
//   if (_label != nil) {
//     return;
//   }

// }

// - (void)refreshUiState {
//   [self setupUiView];
//   _label.stringValue = self.audioUnit != nil
//                            ? @"Hello from Wrapper AUv3 static library"
//                            : @"Loading Wrapper AUv3 UI...";
// }

// - (void)cleanupUiView {
// }

// @end
