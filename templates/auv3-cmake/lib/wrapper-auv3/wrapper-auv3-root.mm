#import "./wrapper-auv3-root.h"
#include "../common/interfaces.h"
#include "../common/parameter_builder_impl.h"
#include "../common/plugin_domain.h"
#include "../common/synthesizer_base.h"
#include "./mac_web_view.h"
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudioKit/CoreAudioKit.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <objc/NSObject.h>
#include <vector>

using namespace sonic_common;

static AUParameter *createAUParameterFromItem(const ParameterItem &entry) {
  AudioUnitParameterOptions paramOptions =
      kAudioUnitParameterFlag_IsWritable | kAudioUnitParameterFlag_IsReadable;
  if (entry.type == ParameterType::Enum) {
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

// --- AUv3ParameterManager Implementation ---
class AUv3ParameterIo : public IPlatformParameterIo {
public:
  AUv3ParameterIo(WrapperAuv3AudioUnit *wrapper) : _wrapper(wrapper) {}

  void setParameterChangeCallback(
      std::function<void(uint64_t, double)> fn) override {
    _onParameterChange = fn;
  }

  void registerParameters(std::vector<ParameterItem> &params) override {
    NSMutableArray *auParams = [NSMutableArray array];
    for (const auto &entry : params) {
      [auParams addObject:createAUParameterFromItem(entry)];
    }

    _parameterTree = [AUParameterTree createTreeWithChildren:auParams];

    AUv3ParameterIo *selfPtr = this;
    _parameterTree.implementorValueObserver =
        ^(AUParameter *param, AUValue value) {
          if (selfPtr->_onParameterChange) {
            selfPtr->_onParameterChange(param.address, (double)value);
          }
        };

    //    _parameterTree.implementorValueProvider = ^(AUParameter *param) {
    //      return param.value;
    //    };
  }

  double getParameter(uint64_t address) override {
    AUParameter *param = [_parameterTree parameterWithAddress:address];
    return param ? (double)param.value : 0.0;
  }

  void setParameter(uint64_t address, double value) override {
    AUParameter *param = [_parameterTree parameterWithAddress:address];
    if (param) {
      param.value = (float)value;
    }
  }

  AUParameterTree *getParameterTree() const { return _parameterTree; }

private:
  __unsafe_unretained WrapperAuv3AudioUnit *_wrapper;
  AUParameterTree *_parameterTree = nil;
  std::function<void(uint64_t, double)> _onParameterChange = nullptr;
};

@interface WrapperAuv3AudioUnit () {
  AUAudioUnitBus *_outputBus;
  AUAudioUnitBusArray *_outputBusArray;
  AUAudioUnitBusArray *_inputBusArray;
  std::unique_ptr<SynthesizerBase> _synth;
  std::unique_ptr<AUv3ParameterIo> _parameterIo;
  std::unique_ptr<PluginDomain> _pluginDomain;
}
@end

@implementation WrapperAuv3AudioUnit

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

  _synth = std::unique_ptr<SynthesizerBase>(createSynthesizerInstance());
  _parameterIo = std::make_unique<AUv3ParameterIo>(self);
  _pluginDomain = std::make_unique<PluginDomain>(*_synth, *_parameterIo);
  _pluginDomain->initialize();

  AVAudioFormat *defaultFormat =
      [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0
                                                     channels:2];
  _outputBus = [[AUAudioUnitBus alloc] initWithFormat:defaultFormat
                                                error:outError];
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
  if (_pluginDomain) {
    _pluginDomain->getDesiredEditorSize(*width, *height);
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
  _pluginDomain->prepareProcessing(sampleRate, maxFrameLength);
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
  PluginDomain *pluginDomainPtr = _pluginDomain.get();
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
          pluginDomainPtr->noteOn(data1, (double)data2 / 127.0);
        } else if (status == 0x80 || (status == 0x90 && data2 == 0)) {
          pluginDomainPtr->noteOff(data1);
        }
      } else if (event->head.eventType == AURenderEventParameter) {
        pluginDomainPtr->setParameter(event->parameter.parameterAddress,
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
    pluginDomainPtr->processAudio(left, right, frameCount);

    return noErr;
  };

  return [block copy];
}

@end

//------------------------------------------------------------

@interface WrapperAuv3ViewFrame () {
  std::unique_ptr<MacWebView> _webView;
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
    _webView.reset();
  }
}

- (void)dealloc {
  [self disconnectViewFromAudioUnit];
#if !__has_feature(objc_arc)
  [super dealloc];
#endif
}

@end
