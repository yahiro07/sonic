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

## Modules Structure

```cpp
//framework
enum ParameterFlags : int {
  None = 0,
  IsReadOnly = 1,
  IsHidden = 2,
  NonAutomatable = 4,
  // Local parameters; not exposed to the host; saved in state or presets
  IsLocal = 8,
};

class ParameterBuilder {
  using Str = std::string_view;
  using StrVec = const std::vector<std::string> &;

  virtual void addFloat(uint32_t id, Str paramKey, Str label,
                        double defaultValue, double minValue = 0.0,
                        double maxValue = 1.0, Str group = "",
                        ParameterFlags flags = ParameterFlags::None) = 0;
  virtual void addEnum(uint32_t id, Str paramKey, Str label,
                       Str defaultValueString, StrVec valueStrings,
                       Str group = "",
                       ParameterFlags flags = ParameterFlags::None) = 0;
  virtual void addBool(uint32_t id, Str paramKey, Str label, bool defaultValue,
                       Str group = "",
                       ParameterFlags flags = ParameterFlags::None) = 0;
};

struct Event {
  // here is a minimum set of events for audio processor
  // we assume it is extended to support advanced specs
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

// basic processor interface, user don't have to care about frame splitting or
// in-frame event list handling
class IBasicAudioProcessor : public IAudioProcessor {
  virtual void prepareProcessing(double sampleRate, uint32_t maxFrameCount) = 0;

  // setParameter, noteOn, noteOff, processAudio are always called in audio
  // thread, they never called from main thread
  // setParameter, noteOn, noteOff are called in sample accurate position
  virtual void setParameter(uint32_t id, double value) = 0;
  virtual void noteOn(int noteNumber, double velocity) = 0;
  virtual void noteOff(int noteNumber) = 0;
  virtual void processAudio(float *bufferL, float *bufferR,
                            uint32_t frames) = 0;
};

// advanced processor interface, user can manually handle frame events and block
// processing
class IAdvancedAudioProcessor : public IAudioProcessor {
  virtual void prepareProcessing(double sampleRate, uint32_t maxFrameCount) = 0;
  virtual void processAudio(float *bufferL, float *bufferR, uint32_t frames,
                            const ProcessData *data) {}
};

class IEditorView {
  virtual void getDesiredSize(int &width, int &height) = 0;
  virtual void setup() = 0;
  virtual void teardown() = 0;
  virtual void attachToParent(void *parent) = 0;
  virtual void removeFromParent() = 0;
  virtual void setBounds(int x, int y, int width, int height) = 0;
};

class IControllerKernel {
public:
  // all methods are assumed to be called in non-audio thread
  virtual double getParameterByHost(uint32_t id) = 0;
  virtual void setParameterByHost(uint32_t id, double value) = 0;
  virtual void
  getAllParametersForPersist(std::map<std::string, double> &parameters) = 0;
  virtual void
  setParametersForPersist(const std::map<std::string, double> &parameters) = 0;

  // implementation is provided by framework
};

class IMainController {

  virtual void setupParameters(ParameterBuilder &builder) = 0;

  virtual IEditorView *createEditorView() = 0;
  virtual double getParameterByHost(uint32_t id) = 0;
  virtual void setParameterByHost(uint32_t id, double value) = 0;

  virtual void saveState(std::vector<uint8_t> &buffer) = 0;
  virtual void restoreState(const std::vector<uint8_t> &buffer) = 0;
};

class WebViewEditorView : public IEditorView {
public:
  static WebViewEditorView *create(IControllerKernel &kernel);
  void setPageUrl(const char *url);
  void setDesiredSize(int width, int height);

  // implementation is provided by framework
};

class DefaultPersistLoader {
public:
  std::map<std::string, double> parameters;
  void emit(std::vector<uint8_t> &buffer);
  void load(const std::vector<uint8_t> &buffer);

  // implementation is provided by framework
};

class IPluginFactory {
  virtual IAudioProcessor *createAudioProcessor() = 0;
  virtual IMainController *createMainController(IControllerKernel &kernel) = 0;
};
IPluginFactory *createPluginFactory();

//----------
// user code

class Project1AudioProcessor : public IAdvancedAudioProcessor {
public:
  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) override {}
  void processAudio(float *bufferL, float *bufferR, uint32_t frames,
                    const ProcessData *data) override {}
};

class Project1Controller : public IMainController {
private:
  IControllerKernel &kernel;

public:
  Project1Controller(IControllerKernel &kernel) : kernel(kernel) {}

  void setupParameters(ParameterBuilder &builder) override {}

  IEditorView *createEditorView() override {
    auto view = WebViewEditorView::create(kernel);
    view->setPageUrl("https://localhost:3000");
    view->setDesiredSize(800, 600);
    return view;
  }

  double getParameterByHost(uint32_t id) override {
    return kernel.getParameterByHost(id);
  }
  void setParameterByHost(uint32_t id, double value) override {
    kernel.setParameterByHost(id, value);
  }

  void saveState(std::vector<uint8_t> &buffer) override {
    DefaultPersistLoader persistLoader;
    kernel.getAllParametersForPersist(persistLoader.parameters);
    persistLoader.emit(buffer);
  }

  void restoreState(const std::vector<uint8_t> &buffer) override {
    DefaultPersistLoader persistLoader;
    persistLoader.load(buffer);
    kernel.setParametersForPersist(persistLoader.parameters);
  }
};

class Project1Factory : public IPluginFactory {
public:
  IAudioProcessor *createAudioProcessor() override {
    return new Project1AudioProcessor();
  }
  IMainController *createMainController(IControllerKernel &kernel) override {
    return new Project1Controller(kernel);
  }
};

IPluginFactory *createPluginFactory() { return new Project1Factory(); }
```
