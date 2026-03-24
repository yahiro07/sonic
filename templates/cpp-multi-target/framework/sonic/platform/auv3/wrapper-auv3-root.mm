#import "./wrapper-auv3-root.h"
#import "./logic/entry-controller.h"
#import "./support/au-parameter-helper.h"
#import "./support/parameter-tree-wrapper.h"
#include "logic/events.h"
#import <AudioToolbox/AUParameters.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudioKit/CoreAudioKit.h>
#import <CoreFoundation/CoreFoundation.h>
#include <memory>
#import <sonic/api/synthesizer-base.h>
#import <sonic/common/logger.h>
#include <sonic/core/editor-factory-registry.h>
#import <sonic/core/editor-interfaces.h>

#if !__has_feature(objc_arc)
#error                                                                         \
    "wrapper-auv3-root.mm requires ARC. Enable -fobjc-arc for Objective-C++ sources."
#endif

using namespace sonic;

@interface WrapperAuv3AudioUnit ()
@property AUParameterTree *parameterTree;
@property AUAudioUnitBus *outputBus;
@property AUAudioUnitBusArray *outputBusArray;
@property AUAudioUnitBusArray *inputBusArray;
@property(strong, nonatomic) NSTimer *eventTimer;
@property(assign, nonatomic) NSInteger activeViewCount;
- (void)startEventLoop;
- (void)stopEventLoop;
- (void)onTimer;
@end

@implementation WrapperAuv3AudioUnit {
  std::unique_ptr<SynthesizerBase> _synth;
  std::unique_ptr<ParameterTreeWrapper> _parameterTreeWrapper;
  std::unique_ptr<EntryController> _entryController;
}
@synthesize parameterTree = _parameterTree;

- (void)setupSynth {
  logger.start();
  logger.mark("setupSynth 1318");
  _synth.reset(createSynthesizerInstance());
  auto parameterItems = EntryController::preGenerateParameterItems(*_synth);
  _parameterTree = createAUParameterTreeFromParameterItems(parameterItems);
  _parameterTreeWrapper =
      ParameterTreeWrapper::create((__bridge void *)_parameterTree);
  _entryController.reset(
      new EntryController(*_synth, parameterItems, *_parameterTreeWrapper));
  _entryController->initialize();

  printf("setupSynth, done\n");
}

- (IControllerFacade *)getControllerFacade {
  return &_entryController->getControllerFacade();
}

- (instancetype)
    initWithComponentDescription:(AudioComponentDescription)componentDescription
                         options:(AudioComponentInstantiationOptions)options
                           error:(NSError **)outError {
  printf("initWithComponentDescription\n");
  self = [super initWithComponentDescription:componentDescription
                                     options:options
                                       error:outError];
  if (!self) {
    return nil;
  }
  [self setupBuses];
  [self setupSynth];

  return self;
}

- (void)dealloc {
  [self stopEventLoop];
  _entryController->terminate();
  _entryController.reset();
  _parameterTreeWrapper.reset();
  _synth.reset();
  logger.stop();
}

- (void)setupBuses {
  AVAudioFormat *defaultFormat =
      [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0
                                                     channels:2];
  _outputBus = [[AUAudioUnitBus alloc] initWithFormat:defaultFormat error:nil];
  _outputBus.maximumChannelCount = 2;
  _outputBusArray =
      [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                             busType:AUAudioUnitBusTypeOutput
                                              busses:@[ _outputBus ]];
  _inputBusArray =
      [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                             busType:AUAudioUnitBusTypeInput
                                              busses:@[]];
}

- (SynthesizerBase *)getSynthesizerInstance {
  return _synth.get();
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
  _synth->prepareProcessing(sampleRate, maxFrameLength);
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
  SynthesizerBase *synthPtr = _synth.get();
  EntryController *controllerPtr = _entryController.get();
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

    {
      UpstreamEvent e;
      while (controllerPtr->popUpstreamEvent(e)) {
        if (e.type == UpstreamEventType::NoteRequest) {
          if (e.note.velocity > 0.f) {
            synthPtr->noteOn(e.note.noteNumber, e.note.velocity);
          } else {
            synthPtr->noteOff(e.note.noteNumber);
          }
        }
      }
    }

    // Handle MIDI and Parameter events
    const AURenderEvent *event = realtimeEventListHead;
    while (event != nullptr) {
      if (event->head.eventType == AURenderEventMIDI) {
        uint8_t status = event->MIDI.data[0] & 0xF0;
        uint8_t data1 = event->MIDI.data[1];
        uint8_t data2 = event->MIDI.data[2];

        if (status == 0x90 && data2 > 0) {
          synthPtr->noteOn(data1, data2 / 127.0);
          controllerPtr->pushDownstreamEvent(DownstreamEvent{
              .type = DownstreamEventType::HostNote,
              .note = {data1, data2 / 127.0},
          });
        } else if (status == 0x80 || (status == 0x90 && data2 == 0)) {
          synthPtr->noteOff(data1);
          controllerPtr->pushDownstreamEvent(DownstreamEvent{
              .type = DownstreamEventType::HostNote,
              .note = {data1, 0.0},
          });
        }
      } else if (event->head.eventType == AURenderEventParameter) {
        auto id = (int32_t)event->parameter.parameterAddress;
        synthPtr->setParameter(id, event->parameter.value);
      }
      event = event->head.next;
    }

    if (frameCount > maxFramesToRender) {
      return kAudioUnitErr_TooManyFramesToProcess;
    }

    // process audio
    AudioBuffer *bufferL = &outputData->mBuffers[0];
    AudioBuffer *bufferR = &outputData->mBuffers[1];
    float *left = (float *)bufferL->mData;
    float *right = (float *)bufferR->mData;
    // debugFillNoise(left, right, frameCount);
    synthPtr->processAudio(left, right, frameCount);

    return noErr;
  };

  return [block copy];
}

- (void)viewAdded {
  self.activeViewCount += 1;
  if (self.activeViewCount == 1) {
    [self startEventLoop];
  }
}

- (void)viewRemoved {
  if (self.activeViewCount == 0) {
    return;
  }

  self.activeViewCount -= 1;
  if (self.activeViewCount == 0) {
    [self stopEventLoop];
  }
}

- (void)startEventLoop {
  if (self.eventTimer) {
    return;
  }

  __weak __typeof__(self) weakSelf = self;
  self.eventTimer = [NSTimer
      timerWithTimeInterval:1.0 / 60.0
                    repeats:YES
                      block:^(NSTimer *_Nonnull timer) {
                        __strong __typeof__(weakSelf) strongSelf = weakSelf;
                        if (!strongSelf) {
                          [timer invalidate];
                          return;
                        }

                        [strongSelf onTimer];
                      }];
  [[NSRunLoop mainRunLoop] addTimer:self.eventTimer
                            forMode:NSRunLoopCommonModes];
}

- (void)stopEventLoop {
  [self.eventTimer invalidate];
  self.eventTimer = nil;
}

- (void)onTimer {
  if (_entryController) {
    _entryController->onTimer();
  }
}

@end

//------------------------------------------------------------

@interface WrapperAuv3ViewFrame () {
  std::unique_ptr<IEditorInstance> _editorInstance;
  WrapperAuv3AudioUnit *_audioUnit;
}
@end

@implementation WrapperAuv3ViewFrame

static std::unique_ptr<IEditorInstance>
setupEditorInstance(std::string url, IControllerFacade &controllerFacade) {
  auto variantName = url.substr(0, url.find(":"));
  if (variantName == "http" || variantName == "https") {
    variantName = "webview";
  }
  printf("setupEditorInstance called, variantName: %s\n", variantName.c_str());

  auto editorFactory =
      EditorFactoryRegistry::getInstance()->getEditorFactory(variantName);
  if (!editorFactory) {
    printf("editor factory not found for variant: %s\n", variantName.c_str());
    return nullptr;
  }
  auto editorInstance = editorFactory(controllerFacade);
  if (variantName == "webview") {
    editorInstance->setup(url);
  } else {
    auto loadTargetSpec = url.substr(url.find(":") + 1);
    editorInstance->setup(loadTargetSpec);
  }
  return editorInstance;
}

- (void)connectViewToAudioUnit:(WrapperAuv3AudioUnit *)audioUnit
                viewController:(AUViewController *)viewController {
  printf("WrapperAuv3ViewFrame connectViewToAudioUnit\n");

  _audioUnit = audioUnit;

  NSView *root = viewController.view;
  root.wantsLayer = YES;
  root.layer.backgroundColor = [[NSColor colorWithCalibratedWhite:0.1
                                                            alpha:1.0] CGColor];
  root.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  root.autoresizesSubviews = YES;

  auto synth = [audioUnit getSynthesizerInstance];
  auto controllerFacade = [audioUnit getControllerFacade];

  auto url = synth->getEditorPageUrl();
  _editorInstance = setupEditorInstance(url, *controllerFacade);
  _editorInstance->attachToParent((__bridge void *)root);

  uint32_t width = 0;
  uint32_t height = 0;
  synth->getDesiredEditorSize(width, height);
  if (width && height) {
    viewController.preferredContentSize = NSMakeSize(width, height);
  }

  [audioUnit viewAdded];
}

- (void)disconnectViewFromAudioUnit {
  printf("WrapperAuv3ViewFrame disconnectViewFromAudioUnit\n");

  if (_audioUnit) {
    [_audioUnit viewRemoved];
    _audioUnit = nil;
  }

  if (_editorInstance) {
    _editorInstance->removeFromParent();
    _editorInstance->teardown();
    _editorInstance.reset();
  }
}

- (void)dealloc {
  [self disconnectViewFromAudioUnit];
}

@end
