#import "./wrapper-auv3-root.h"
#import "../api/synthesizer-base.h"
#import "../common/logger.h"
#import "../common/mac-web-view.h"
#import "../domain/webview-bridge.h"
#import "./au-parameter-helper.h"
#import "./entry-controller.h"
#import "./parameter-tree-wrapper.h"
#import <AudioToolbox/AUParameters.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudioKit/CoreAudioKit.h>
#import <CoreFoundation/CoreFoundation.h>

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
@end

@implementation WrapperAuv3AudioUnit {
  std::unique_ptr<SynthesizerBase> _synth;
  std::unique_ptr<ParameterTreeWrapper> _parameterTreeWrapper;
  std::unique_ptr<EntryController> _entryController;
}
@synthesize parameterTree = _parameterTree;

- (void)setupSynth {
  logger.start();
  logger.mark("setupSynth 0837");
  _synth.reset(createSynthesizerInstance());
  auto parameterItems = EntryController::preGenerateParameterItems(*_synth);
  _parameterTree = createAUParameterTreeFromParameterItems(parameterItems);
  _parameterTreeWrapper =
      ParameterTreeWrapper::create((__bridge void *)_parameterTree);
  _entryController =
      std::make_unique<EntryController>(*_synth, *_parameterTreeWrapper);
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
  printf("initWithComponentDescription 0930\n");
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

- (void)getDesiredEditorSize:(uint32_t *)width height:(uint32_t *)height {
  if (_synth) {
    _synth->getDesiredEditorSize(*width, *height);
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
    const AURenderEvent *event = realtimeEventListHead;
    while (event != nullptr) {
      if (event->head.eventType == AURenderEventMIDI) {
        uint8_t status = event->MIDI.data[0] & 0xF0;
        uint8_t data1 = event->MIDI.data[1];
        uint8_t data2 = event->MIDI.data[2];

        if (status == 0x90 && data2 > 0) {
          synthPtr->noteOn(data1, (double)data2 / 127.0);
        } else if (status == 0x80 || (status == 0x90 && data2 == 0)) {
          synthPtr->noteOff(data1);
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

@end

//------------------------------------------------------------

@interface WrapperAuv3ViewFrame () {
  std::unique_ptr<MacWebView> _webView;
  std::unique_ptr<WebViewBridge> _webViewBridge;
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
  root.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  root.autoresizesSubviews = YES;

  if (!_webView) {
    _webView = std::make_unique<MacWebView>();
    _webView->attachToParent((__bridge void *)root);
    _webView->loadUrl("http://localhost:3000");
  }
  if (!_webViewBridge) {
    auto controllerFacade = [audioUnit getControllerFacade];
    auto webViewIo = (IWebViewIo *)_webView.get();
    _webViewBridge = std::unique_ptr<WebViewBridge>(
        WebViewBridge::create(*controllerFacade, *webViewIo));
    _webViewBridge->setup();
  }

  uint32_t width = 0;
  uint32_t height = 0;
  [audioUnit getDesiredEditorSize:&width height:&height];
  if (width && height) {
    viewController.preferredContentSize = NSMakeSize(width, height);
  }
}

- (void)disconnectViewFromAudioUnit {
  printf("WrapperAuv3ViewFrame disconnectViewFromAudioUnit\n");
  if (_webViewBridge) {
    _webViewBridge->teardown();
    _webViewBridge.reset();
  }
  if (_webView) {
    _webView->removeFromParent();
    _webView.reset();
  }
}

- (void)dealloc {
  [self disconnectViewFromAudioUnit];
}

@end
