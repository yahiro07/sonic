# Separated Model Proposal

Currently, the `cpp-multi-target` template project offers a simple structure that is easy to implement, but it lacks extensibility and less suitable for developing complex plugins. Therefore, we are exploring a more extensible architecture.

The following challenges exist:

- Separation of DSP processing and the controller
- Enabling DSP processing to handle the event data list
- Designing the system to support views other than WebView
- Allowing the controller to customize parameter routing
- Allowing the serialization of state to be overridden
- Allowing user code to manage and persist arbitrary state other than parameters
- Allowing arbitrary custom commands and data to be sent from the UI to DSP processing

## Enabling DSP processing to handle the event data list

### Raw Interfaces

```cpp
  virtual void processAudio(float *bufferL, float *bufferR,
    uint32_t frames, ProcessData *data) {}
```

### Wrapped Interfaces

```cpp
  virtual void setParameter(uint32_t id, double value) = 0;
  virtual void noteOn(int noteNumber, double velocity) = 0;
  virtual void noteOff(int noteNumber) = 0;
  virtual void processAudio(float *bufferL, float *bufferR,
    uint32_t frames) = 0;
```

We will provide two interfaces: one that simply relays the list of events from the host, and another that wraps this and splits and processes the frames internally. The Wrapped Interface is sufficient for tasks that do not involve complex processing, but the Raw Interface should be used if you want to handle frame splitting and chunk processing yourself. The controller will be designed to include a method that returns a processor implementing these interfaces.

## 構成案

### framework

```cpp
class ParameterBuilder;

struct Event {
  enum class EventType {
    NoteOn,
    NoteOff,
    ParameterChange,
  } type;
  union {
    struct {
      int noteNumber;
      double velocity;
    } note;
    struct {
      uint32_t id;
      double value;
    } parameter;
  };
  int sampleOffset;
};

struct ProcessData {
  int numEvents;
  const Event *events;
};

class IAudioProcessor {};

class BasicAudioProcessor : public IAudioProcessor {
  virtual void prepareProcessing(double sampleRate, uint32_t maxFrameCount) = 0;
  virtual void setParameter(uint32_t id, double value) = 0;
  virtual void noteOn(int noteNumber, double velocity) = 0;
  virtual void noteOff(int noteNumber) = 0;
  virtual void processAudio(float *bufferL, float *bufferR,
                            uint32_t frames) = 0;
};

class AdvancedAudioProcessor : public IAudioProcessor {
  virtual void prepareProcessing(double sampleRate, uint32_t maxFrameCount) = 0;
  virtual void processAudio(float *bufferL, float *bufferR, uint32_t frames,
                            ProcessData *data) {}
};

class IEditorView {
  virtual void getDesiredSize(int &width, int &height) = 0;
  virtual void setup() = 0;
  virtual void teardown() = 0;
  virtual void attachToParent(void *parent) = 0;
  virtual void removeFromParent() = 0;
  virtual void setBounds(int x, int y, int width, int height) = 0;
};

class WebViewEditorView : public IEditorView {
public:
  static WebViewEditorView *create(const char *url, int desiredWidth,
                                   int desiredHeight);
};

class UnitController {
  virtual void setupParameters(ParameterBuilder &builder) = 0;
  virtual IAudioProcessor *getAudioProcessor() = 0;
  virtual IEditorView *createEditorView() = 0;

  // The interface for managing parameters on the main thread and for persisting/restoring state is
  // currently under consideration
};
```

### user code

```cpp

class Project1AudioProcessor : public AdvancedAudioProcessor {
public:
  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) override {}
  void processAudio(float *bufferL, float *bufferR, uint32_t frames,
                    ProcessData *data) override {}
};

class Project1Controller : public UnitController {
  Project1AudioProcessor audioProcessor;
  void setupParameters(ParameterBuilder &builder) override {}
  IAudioProcessor *getAudioProcessor() override { return &audioProcessor; }
  IEditorView *createEditorView() override {
    return WebViewEditorView::create("https://localhost:3000", 800, 600);
  }
};

UnitController *createUnitController() { return new Project1Controller(); }
```
