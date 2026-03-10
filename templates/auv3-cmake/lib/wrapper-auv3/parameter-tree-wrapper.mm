#include "./parameter-tree-wrapper.h"
#import <CoreAudioKit/CoreAudioKit.h>

namespace sonic {

class ParameterTreeWrapper : public IParameterTreeWrapper {
private:
  AUParameterTree *_parameterTree;

  bool hasValidParameterTree() const {
    return _parameterTree &&
           [_parameterTree isKindOfClass:[AUParameterTree class]];
  }

public:
  ParameterTreeWrapper(AUParameterTree *parameterTree)
      : _parameterTree(parameterTree) {
    if (_parameterTree) {
      CFRetain((__bridge CFTypeRef)_parameterTree);
    }
    if (!hasValidParameterTree()) {
      const char *className =
          _parameterTree ? object_getClassName(_parameterTree) : "(null)";
      printf("invalid parameter tree: ptr=%p class=%s\n", _parameterTree,
             className);
      return;
    }
  }
  ~ParameterTreeWrapper() {
    if (_parameterTree) {
      CFRelease((__bridge CFTypeRef)_parameterTree);
      _parameterTree = nil;
    }
  }

  void setImplementorValueObserver(
      std::function<void(uint64_t address, float value)> observer) override {
    _parameterTree.implementorValueObserver =
        ^(AUParameter *param, AUValue value) {
          observer(param.address, value);
        };
  }
  virtual void setImplementorValueProvider(
      std::function<float(uint64_t address)> provider) override {
    _parameterTree.implementorValueProvider = ^(AUParameter *param) {
      return provider(param.address);
    };
  }

  virtual void
  setParameterValue(uint64_t address, float value, void *originator,
                    ParameterAutomationEventType eventType) override {
    AUParameter *param = [_parameterTree parameterWithAddress:address];
    if (param) {
      [param setValue:value
           originator:originator
           atHostTime:0
            eventType:(AUParameterAutomationEventType)eventType];
    }
  }
  virtual float getParameterValue(uint64_t address) override {
    AUParameter *param = [_parameterTree parameterWithAddress:address];
    return param ? [param value] : 0.0;
  }

  virtual void *tokenByAddingParameterObserver(
      std::function<void(uint64_t address, float value)> observer) override {
    return [_parameterTree tokenByAddingParameterObserver:^(
                               AUParameterAddress address, AUValue value) {
      observer(address, value);
    }];
  }
  virtual void removeParameterObserver(void *observerToken) override {
    [_parameterTree removeParameterObserver:observerToken];
  }
};

IParameterTreeWrapper *createParameterTreeWrapper(void *parameterTree) {
  return new ParameterTreeWrapper((__bridge AUParameterTree *)parameterTree);
}

void destroyParameterTreeWrapper(IParameterTreeWrapper *wrapper) {
  delete wrapper;
}

} // namespace sonic