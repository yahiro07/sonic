#import "./wrapper-auv3-root.h"
#import "../api/synthesizer-base.h"
#import "../common/logger.h"
#import "../common/mac-web-view.h"
#import "../core/parameter-builder-impl.h"
#import "../core/parameter-definitions-provider.h"
#import "../domain/interfaces.h"
#import "../domain/parameters-store.h"
#import "../domain/webview-bridge.h"
#import "./controller-parameter-port.h"
#import "./parameter-tree-wrapper.h"
#import <AudioToolbox/AUParameters.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudioKit/CoreAudioKit.h>
#import <CoreFoundation/CoreFoundation.h>
#import <cstdio>
#import <cstdlib>
#import <cstring>
#import <memory>
#import <objc/NSObject.h>
#import <objc/runtime.h>
#import <vector>

using namespace sonic;

static AUParameter *
createAUParameterFromParameterItem(const ParameterItem &entry) {
  AudioUnitParameterOptions paramOptions =
      kAudioUnitParameterFlag_IsWritable | kAudioUnitParameterFlag_IsReadable;
  if (entry.type == ParameterType::Enum) {
    paramOptions |= kAudioUnitParameterFlag_ValuesHaveStrings;
  }
  AUParameter *param = [AUParameterTree
      createParameterWithIdentifier:[NSString
                                        stringWithUTF8String:entry.paramKey
                                                                 .c_str()]
                               name:[NSString stringWithUTF8String:entry.label
                                                                       .c_str()]
                            address:entry.id
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

static AUParameterTree *createAUParameterTreeFromParameterItems(
    const std::vector<ParameterItem> &items) {
  NSMutableArray *auParams = [NSMutableArray array];
  for (const auto &entry : items) {
    [auParams addObject:createAUParameterFromParameterItem(entry)];
  }
  return [AUParameterTree createTreeWithChildren:auParams];
}

static int getMaxIdFromParameterItems(const std::vector<ParameterItem> &items) {
  int maxId = 0;
  for (const auto &item : items) {
    if (item.id > maxId) {
      maxId = item.id;
    }
  }
  return maxId;
}

class ControllerFacade : public IControllerFacade {
private:
  ControllerParameterPort &parameterPort;

public:
  ControllerFacade(ControllerParameterPort &parameterPort)
      : parameterPort(parameterPort) {}

  int subscribeParameterChange(
      std::function<void(const std::string, double)> callback) override {
    return parameterPort.subscribeToParameterChanges(callback);
  }

  void unsubscribeParameterChange(int token) override {
    parameterPort.unsubscribeFromParameterChanges(token);
  }

  void applyParameterEditFromUi(std::string paramKey, double value,
                                ParameterEditState editState) override {
    parameterPort.applyParameterEditFromUi(paramKey, value, editState);
  }

  void getAllParameters(std::map<std::string, double> &parameters) override {
    parameterPort.getAllParameters(parameters);
  }

  void requestNoteOn(int noteNumber, double velocity) override {}
  void requestNoteOff(int noteNumber) override {}
};

@interface WrapperAuv3AudioUnit ()
@property AUParameterTree *parameterTree;
@property AUAudioUnitBus *outputBus;
@property AUAudioUnitBusArray *outputBusArray;
@property AUAudioUnitBusArray *inputBusArray;
@end

@implementation WrapperAuv3AudioUnit {
  SynthesizerBase *_synth;
  ParameterDefinitionsProvider *_parametersDefinitionProvider;
  ParametersStore *_parametersStore;
  IParameterTreeWrapper *_parameterTreeWrapper;
  ControllerParameterPort *_controllerParameterPort;
  ControllerFacade *_controllerFacade;
}
@synthesize parameterTree = _parameterTree;

- (void)setupSynth {
  logger.start();
  logger.mark("setupSynth 0238");
  _synth = createSynthesizerInstance();
  ParameterBuilderImpl builder;
  _synth->setupParameters(builder);
  auto parameterItems = builder.getItems();
  _parameterTree = createAUParameterTreeFromParameterItems(parameterItems);
  _parameterTreeWrapper =
      createParameterTreeWrapper((__bridge void *)_parameterTree);
  _parametersDefinitionProvider = new ParameterDefinitionsProvider();
  _parametersDefinitionProvider->addParameters(parameterItems, 0xFFFFFFFF);
  _parametersStore = new ParametersStore();
  auto maxId = getMaxIdFromParameterItems(parameterItems);
  _parametersStore->setup(maxId);
  for (const auto &item : parameterItems) {
    _parametersStore->set(item.id, item.defaultValue);
  }
  ParametersStore *parameterStorePtr = _parametersStore;
  SynthesizerBase *synthPtr = _synth;
  [_parameterTree
      setImplementorValueObserver:^(AUParameter *param, AUValue value) {
        auto id = (int32_t)param.address;
        parameterStorePtr->set(id, value);
        synthPtr->setParameter(id, (double)value);
      }];
  [_parameterTree setImplementorValueProvider:^(AUParameter *param) {
    auto id = (int32_t)param.address;
    return (float)parameterStorePtr->get(id);
  }];
  _controllerParameterPort = new ControllerParameterPort(
      *_parameterTreeWrapper, *_parametersDefinitionProvider);
  _controllerFacade = new ControllerFacade(*_controllerParameterPort);

  printf("setupSynth, done\n");
}

- (IControllerFacade *)getControllerFacade {
  return (IControllerFacade *)_controllerFacade;
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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
- (void)dealloc {
  delete _controllerFacade;
  _controllerFacade = nullptr;
  delete _controllerParameterPort;
  _controllerParameterPort = nullptr;
  destroyParameterTreeWrapper(_parameterTreeWrapper);
  _parameterTreeWrapper = nullptr;
  delete _parametersStore;
  _parametersStore = nullptr;
  delete _parametersDefinitionProvider;
  _parametersDefinitionProvider = nullptr;
  delete _synth;
  _synth = nullptr;
  logger.stop();
#if !__has_feature(objc_arc)
  [super dealloc];
#endif
}
#pragma clang diagnostic pop

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
  SynthesizerBase *synthPtr = _synth;
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
  MacWebView *_webView;
  WebViewBridge *_webViewBridge;
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
    _webView = new MacWebView();
    _webView->attachToParent((__bridge void *)root);
    _webView->loadUrl("http://localhost:3000");
  }
  if (!_webViewBridge) {
    auto controllerFacade = [audioUnit getControllerFacade];
    auto webViewIo = (IWebViewIo *)_webView;
    _webViewBridge = new WebViewBridge(*controllerFacade, *webViewIo);
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
    delete _webViewBridge;
    _webViewBridge = nullptr;
  }
  if (_webView) {
    _webView->removeFromParent();
    delete _webView;
    _webView = nullptr;
  }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
- (void)dealloc {
  [self disconnectViewFromAudioUnit];
#if !__has_feature(objc_arc)
  [super dealloc];
#endif
}
#pragma clang diagnostic pop

@end
