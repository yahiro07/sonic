# AdvancedSynthesizerBase

## Overview

This is the base interface for a synthesizer with advanced features currently in the planning stages.
We are currently focusing solely on the interface. There is no wrapper implementation that meets these requirements yet (under development).

We are planning to add the following features:

- Binary parameters used internally by plugins
- Parameter migration between multiple versions
- An interface to retrieve data from arrays held by the DSP core from the UI

This will enable the implementation of MSEG LFO/EG, as well as plugins such as waveform monitors and spectrum analyzers.

```cpp

enum ParameterFlags : int {
  None = 0,
  IsReadOnly = 1,
  IsHidden = 2,
  NonAutomatable = 4,
  // Local parameters; not exposed to the host; saved in State or presets
  IsLocal = 8,
};

class ParameterBuilder {
protected:
  using Str = std::string_view;
  using StrVec = const std::vector<std::string> &;

public:
  virtual ~ParameterBuilder() = default;
  virtual void addUnary(uint64_t address, Str identifier, Str label,
                        double defaultValue, Str group = "",
                        ParameterFlags flags = ParameterFlags::None) = 0;
  virtual void addEnum(uint64_t address, Str identifier, Str label,
                       Str defaultValueString, StrVec valueStrings,
                       Str group = "",
                       ParameterFlags flags = ParameterFlags::None) = 0;
  virtual void addBool(uint64_t address, Str identifier, Str label,
                       bool defaultValue, Str group = "",
                       ParameterFlags flags = ParameterFlags::None) = 0;
  // Variable-length binary data parameters, intended for things like MSEG point arrays.
  // Not exposed to the host; managed internally.
  virtual void addBinary(uint64_t address, Str identifier,
                         const std::vector<uint8_t> &defaultValue) = 0;
};

struct Event {
  enum class EventType {
    NoteOn,
    NoteOff,
    ParameterChange,
  } type;
  union {
    struct {
      int32_t noteNumber;
      double velocity;
    } note;
    struct {
      uint64_t address;
      double value;
    } parameter;
  };
  int sampleOffset;
};

struct ProcessData {
  int numEvents;
  const Event *events;
};

class TelemetryBuilder {
public:
  // Define a buffer for telemetry data by specifying an ID and array size. IDs range from 0 to 31.
  // Used to send data from the sound source to the UI, such as waveform data or FFT buffers for display.
  // The framework allocates buffers of these sizes, which are used for polling.
  virtual void defineFloatArray(int id, uint32_t count) = 0;
};

class AdvancedSynthesizer: public SynthesizerBase {
public:
  virtual ~AdvancedSynthesizer() = default;
  //declared in base class
  // virtual void setupParameters(ParameterBuilder &builder) = 0;
  // virtual void prepareProcessing(double sampleRate, uint32_t maxFrameCount) = 0;

  // Flag to determine whether to use the low-level processAudioEx.
  // If enabled, processAudioEx will be called for waveform generation, and
  // precessAudio/setParameter/noteOn/noteOff will not be called.
  virtual bool isProcessAudioExSupported() { return false; }
  //
  virtual void processAudioEx(float *bufferL, float *bufferR, uint32_t frames,
                              ProcessData *data) {}

  //declared in base class
  // virtual void processAudio(float *bufferL, float *bufferR, uint32_t frames) = 0;
  // virtual void setParameter(uint64_t address, double value) = 0;
  // virtual void noteOn(int32_t noteNumber, double velocity) = 0;
  // virtual void noteOff(int32_t noteNumber) = 0;
  //

  // Apply binary parameters
  virtual void setBinaryParameter(uint64_t address, const uint8_t *dataBytes,
                                  int dataLength) {}

  // Called when the host's play state or tempo changes
  virtual void setHostPlayState(bool playing, double bpm) {}

  // User-defined custom actions
  // Called on the audio thread; the C++ overload called depends on the message format received from the UI
  virtual void handleCustomAction(int id, double param1, double param2) {}
  virtual void handleCustomAction(int id, uint8_t *dataBytes, int dataLength) {}

  // Receive strings sent from the UI directly; received on the UI thread
  // virtual void __underConsideration__handleDirectMessageFromUi(std::string msg) {}
  // Get single values from the sound source
  // virtual double __underConsideration__pullSingleDataExposed(int id) {}

  // Define telemetry data structures
  virtual void setupTelemetries(TelemetryBuilder &builder) {};
  // Retrieve telemetry data; called within the audio thread
  // The UI polls to get byte array data from the sound source and send it to the WebView
  virtual bool readTelemetry(int id, float *buffer, uint32_t count) {}

  // Parameter migration
  // Return the current latest parameter version as a fixed value (hardcoded)
  virtual int getParametersVersionLatest() { return 0; }
  // Called when presets or state are loaded
  // The sound source's waveform generation process is expected to branch processing by referencing the values set here,
  // thereby maintaining compatibility with older presets.
  virtual void setParametersVersion(int parametersVersion) {}
  // Called when presets or state are loaded. If you intend to rewrite the parameter set itself during loading,
  // handle the processing here.
  // Expected to handle changes in linear parameter mappings, additions to Enum parameter values, etc.
  virtual void migrateParameters(std::map<uint64_t, double> &parameters,
                                 int parametersVersion) {}

  // Override default state persistence implementation
  virtual bool isOverridingStatePersistence() { return false; }
  virtual void readStateOverride(BinaryStream &stream) {}
  virtual void writeStateOverride(BinaryStream &stream) {}

  //declared in base class
  // virtual std::string getEditorPageUrl() = 0;
}
```
