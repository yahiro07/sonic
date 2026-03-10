#import "./wrapper-auv3-root.h"
#include "../api/synthesizer-base.h"
#include "../common/mac-web-view.h"
#include "../core/parameter-builder-impl.h"
#include "../core/parameter-definitions-provider.h"
#include "../domain/interfaces.h"
#include "../domain/parameters-store.h"
#include <AudioToolbox/AUParameters.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudioKit/CoreAudioKit.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <objc/NSObject.h>
#include <vector>

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

class ControllerParameterPort {
private:
  ParameterDefinitionsProvider &_parametersDefinitionProvider;
  AUParameterTree *parameterTree;
  AUParameterObserverToken observerToken = 0;

  std::map<int, std::function<void(std::string, double)>> listeners;
  int nextListenerToken = 0;

  void startObserve() {
    observerToken = [parameterTree
        tokenByAddingParameterObserver:^(AUParameterAddress address,
                                         AUValue value) {
          auto param = [parameterTree parameterWithAddress:address];
          if (!param) {
            return;
          }
          auto paramKey = [param.identifier UTF8String];
          for (const auto &[token, listener] : listeners) {
            listener(paramKey, (double)value);
          }
        }];
  }

  void stopObserve() {
    if (observerToken) {
      [parameterTree removeParameterObserver:observerToken];
      observerToken = 0;
    }
  }

public:
  ControllerParameterPort(
      AUParameterTree *parameterTree,
      ParameterDefinitionsProvider &parametersDefinitionProvider)
      : parameterTree(parameterTree),
        _parametersDefinitionProvider(parametersDefinitionProvider) {}

  int subscribeToParameterChanges(
      std::function<void(std::string, double)> listener) {
    int token = nextListenerToken++;
    listeners[token] = listener;
    if (listeners.size() == 1) {
      startObserve();
    }
    return token;
  }

  void unsubscribeFromParameterChanges(int token) {
    listeners.erase(token);
    if (listeners.empty()) {
      stopObserve();
    }
  }

  void applyParameterEditFromUi(std::string paramKey, double value,
                                ParameterEditState editState) {
    auto id = _parametersDefinitionProvider.getIdByParamKey(paramKey);
    if (id == std::nullopt) {
      return;
    }
    auto param = [parameterTree parameterWithAddress:*id];
    if (!param) {
      return;
    }
    if (editState == ParameterEditState::Begin) {
      [param setValue:value
           originator:observerToken
           atHostTime:0
            eventType:AUParameterAutomationEventTypeTouch];
    } else if (editState == ParameterEditState::End) {
      [param setValue:value
           originator:observerToken
           atHostTime:0
            eventType:AUParameterAutomationEventTypeRelease];
    } else if (editState == ParameterEditState::Perform) {
      [param setValue:value];
    } else if (editState == ParameterEditState::InstantChange) {
      [param setValue:value];
    }
  }

  void getAllParameters(std::map<std::string, double> &parameters) {
    auto parameterItems = _parametersDefinitionProvider.getParameterItems();
    for (const auto &item : parameterItems) {
      AUParameter *param = [parameterTree parameterWithAddress:item.id];
      if (param) {
        parameters[item.paramKey] = param.value;
      }
    }
  }
};

class ControllerFacade : IControllerFacade {
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

// @interface WrapperAuv3AudioUnit: AUAudioUnit
// - (ControllerFacade *)getControllerFacade;
// @end

@interface WrapperAuv3AudioUnit () {
  AUAudioUnitBus *_outputBus;
  AUAudioUnitBusArray *_outputBusArray;
  AUAudioUnitBusArray *_inputBusArray;
  SynthesizerBase *_synth;
  ControllerFacade *_controllerFacade;
}
@end

@implementation WrapperAuv3AudioUnit

- (void)setupSynth {
  printf("setupSynth\n");
  _synth = createSynthesizerInstance();
  ParameterBuilderImpl builder;
  _synth->setupParameters(builder);
  auto parameterItems = builder.getItems();
  auto parameterTree = createAUParameterTreeFromParameterItems(parameterItems);
  auto parametersDefinitionProvider = new ParameterDefinitionsProvider();
  parametersDefinitionProvider->addParameters(parameterItems, 0xFFFFFFFF);
  auto parametersStore = new ParametersStore();
  auto maxId = getMaxIdFromParameterItems(parameterItems);
  parametersStore->setup(maxId);
  for (const auto &item : parameterItems) {
    parametersStore->set(item.id, item.defaultValue);
  }
  [parameterTree
      setImplementorValueObserver:^(AUParameter *param, AUValue value) {
        parametersStore->set(param.address, value);
        _synth->setParameter(param.address, (double)value);
      }];
  [parameterTree setImplementorValueProvider:^(AUParameter *param) {
    return (float)parametersStore->get(param.address);
  }];
  auto controllerParameterPort = std::make_unique<ControllerParameterPort>(
      parameterTree, *parametersDefinitionProvider);
  _controllerFacade = new ControllerFacade(*controllerParameterPort);

  printf("setupSynth, done\n");
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

  AVAudioFormat *defaultFormat =
      [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0
                                                     channels:2];
  _outputBus = [[AUAudioUnitBus alloc] initWithFormat:defaultFormat
                                                error:outError];
  if (!_outputBus) {
    return nil;
  }

  [self setupBuses];

  [self setupSynth];

  return self;
}

- (void)setupBuses {
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
        synthPtr->setParameter(event->parameter.parameterAddress,
                               event->parameter.value);
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

  uint32_t width = 0;
  uint32_t height = 0;
  [audioUnit getDesiredEditorSize:&width height:&height];
  if (width && height) {
    viewController.preferredContentSize = NSMakeSize(width, height);
  }
}

- (void)disconnectViewFromAudioUnit {
  printf("WrapperAuv3ViewFrame disconnectViewFromAudioUnit\n");
  if (_webView) {
    _webView->removeFromParent();
    _webView = nullptr;
  }
}

- (void)dealloc {
  [self disconnectViewFromAudioUnit];
}

@end
