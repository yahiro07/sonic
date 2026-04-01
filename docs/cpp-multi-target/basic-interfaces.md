# SynthesizerBase

## Overview

The following section describes the `SynthesizerBase` class, which forms the core of the plugin.
When creating a new plugin, you should inherit from this class to implement it.

```cpp
class SynthesizerBase {
public:
  virtual ~SynthesizerBase() = default;
  virtual void setupParameters(ParameterBuilder &builder) = 0;
  virtual void prepareProcessing(double sampleRate, uint32_t maxFrameCount) = 0;

  virtual void setParameter(uint32_t id, double value) = 0;
  virtual void noteOn(int noteNumber, double velocity) = 0;
  virtual void noteOff(int noteNumber) = 0;
  virtual void processAudio(float *bufferL, float *bufferR,
                            uint32_t frames) = 0;

  virtual void getDesiredEditorSize(uint32_t &width, uint32_t &height) = 0;
  virtual std::string getEditorPageUrl() = 0;
};
```

`setParameter`, `noteOn`, `noteOff`, and `processAudio` are involved in DSP processing and are called on the audio thread. All other methods are called on the main thread. Parameter management on the main thread is not included in the user code; it is handled by the wrapper.

### setupParameters

```cpp
virtual void setupParameters(ParameterBuilder &builder) = 0;
```

When a plugin is loaded by the host, `setupParameters` is called to construct a set of parameter definitions. Plugin parameter definitions are created using an instance of `ParameterBuilder`.

#### Note: About Controller Configuration

In VST3, the processor and controller are implemented as separate classes.
In Sonic's VST3 wrapper implementation, the processor and controller each create their own instance of `SynthesizerBase` and call `setupParameters` individually. When implementing `setupParameters`, ensure that the code has no side effects and simply defines the parameters.

```cpp
enum ParameterAddress {
  kOscEnabled = 0,
  kOscWave,
  kOscPitch,
  kOscVolume,
};

void Project1Synthesizer::setupParameters(ParameterBuilder &builder) {
  builder.addBool(kOscEnabled, "oscEnabled", "Osc Enabled", true);
  builder.addEnum(kOscWave, "oscWave", "Wave Type", "Saw",
                  {"Saw", "Square", "Triangle", "Sine"});
  builder.addFloat(kOscPitch, "oscPitch", "OSC Pitch", 0.5);
  builder.addFloat(kOscVolume, "oscVolume", "OSC Volume", 0.5);
}
```

This is an example of defining several parameters. For more details, see the section [Details on Defining Parameters](#Defining_Parameters).

### prepareProcessing

```cpp
virtual void prepareProcessing(double sampleRate, uint32_t maxFrameCount) = 0;
```

The `prepareProcessing` method is called before actual audio frame processing begins. The sampling rate and maximum buffer size are passed to it. If the plugin requires a buffer for internal processing, it should be allocated here. The `prepareProcessing` method may be called multiple times, depending on the environment.

### setParameter

```cpp
virtual void setParameter(uint32_t id, double value) = 0;
```

This method is called when parameters are changed. It is identified by the parameter ID registered in `setupParameters`. The value of `value` is a denormalized value.

In the Sonic framework, parameters are always sent unidirectionally from the wrapper to the DSP implementation. When the host requests a parameter, the wrapper returns the value it holds.

### noteOn

```cpp
virtual void noteOn(int noteNumber, double velocity) = 0;
```

This function is called when a note on event is received from the host. noteNumber is a value in the MIDI range of 0–127. velocity is a floating-point value in the range of 0.0–1.0.

### noteOff

```cpp
virtual void noteOff(int noteNumber) = 0;
```

This method is called when a note off event is received from the host.

### processAudio

```cpp
virtual void processAudio(float *bufferL, float *bufferR, uint32_t frames) = 0;
```

This method handles the actual processing of audio frames. `bufferL` and `bufferR` represent the audio frames for the left and right channels, respectively. Process the number of samples specified by `frames`.
If the plugin type is “instrument,” the caller fills the buffers with zeros before calling this method. If the plugin type is “effect,” the input waveform is passed in as-is, and the `processAudio` method is intended to replace it and output the result.

#### Note: Processing Blocks and Event Triggering at the Sample Level

The wrapper divides the audio into frames at the boundaries of sample-level events received from the host and processes them accordingly. `noteOn`, `noteOff`, and `setParameter` are called at the sample offsets specified by the host. Therefore, the number of samples processed by the `processAudio` method (the value of `frames`) changes with every call. There is a possibility that calls will be made for waveform processing involving only a single sample; please implement your code to handle this scenario.

### getDesiredEditorSize

```cpp
virtual void getDesiredEditorSize(uint32_t &width, uint32_t &height) = 0;
```

Returns the width and height you want the UI to display when it first appears. Please specify the desired dimensions in `width` and `height`.

### getEditorPageUrl

```cpp
virtual std::string getEditorPageUrl() = 0;
```

Returns the URL of the editor's home page as it appears in the WebView.

#### Load page from resources

```
app://www-bundles/index.html
app://www-bundles/index.html?debug=1
app://www-vanilla/index.html
app://www-vanilla/index.html?debug=1
```

URLs beginning with `app://` are custom scheme URLs provided by the wrapper framework. They map to content files managed as assets by the plugin.

`pages/www-bundles` is where frontend files built with module bundlers such as Vite are placed.
`pages/www-vanilla` is used to store HTML, JS, and CSS files directly without using a bundler.
For `app://`, the host part corresponds to subfolders within the `pages` folder.

Adding the `debug=1` flag to the end of a URL enables frontend debug logs.

#### Load page for development

```
http://localhost:3000
http://localhost:3000?debug=1
```

You can load the page from the dev server of a module bundler such as Vite.
This is not limited to localhost; you can specify and display any URL, such as `https://example.com`.

## Defining Parameters

Plugin parameter sets are constructed using an instance of the `ParameterBuilder` class.
Since this builder is passed to the `setupParameters` method, please call the method to register the parameters.

```cpp
class ParameterBuilder {
protected:
  using Str = std::string_view;
  using StrVec = const std::vector<std::string_view> &;

public:
  virtual ~ParameterBuilder() = default;
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
```

```cpp
// example
void Project1Synthesizer::setupParameters(ParameterBuilder &builder) {
  builder.addBool(kOscEnabled, "oscEnabled", "Osc Enabled", true);
  builder.addEnum(kOscWave, "oscWave", "Wave Type", "Saw",
                  {"Saw", "Square", "Triangle", "Sine"});
  builder.addFloat(kOscPitch, "oscPitch", "OSC Pitch", 0.5);
  builder.addFloat(kOscVolume, "oscVolume", "OSC Volume", 0.5);
```

Each parameter has an ID, key, label, default value, group, and flags.

### Parameter Types

#### Float Parameter

A parameter with a continuous real value.

#### Enum Parameter

A parameter that selects one option from multiple choices. Used when defining oscillator waveform types, filter types (LFP/BPF/HPF), etc.

#### Bool Parameter

A binary parameter such as ON/OFF.

### id

Used to identify the parameter. It is exposed to the host.
The `id` of a parameter is represented by the `uint32_t` type. When targeting VST3, the valid range for `id` values is 0 <= `id` <= 0x7FFFFFFF. Values greater than or equal to 0x80000000 are reserved and cannot be used, so please be careful. The `id` does not need to be a consecutive number such as 0, 1, 2, 3...; you can set any value within the valid range.

### paramKey

This string is used for identification when passing parameters between the plugin itself and the WebView UI.
It is also used as a key when persisting parameters on the plugin side.
Please keep this key as a fixed value and do not change it, even when updating the plugin version.

### label

The display name of the parameter shown on the host side.

### default value

The initial value of the parameter. The type varies depending on the parameter type.

### group

The key used for grouping parameter sets. Parameters with the same key are treated as a group in the host's UI. The implementation depends on the host.

### flags

```cpp
enum ParameterFlags : int {
  None = 0,
  IsReadOnly = 1 << 0,
  IsHidden = 1 << 1,
  NonAutomatable = 1 << 2,
};
```

The flag can take four values: `None`, `IsReadOnly`, `IsHidden`, and `NonAutomatable`. These values are combined using a bitwise OR operator to specify the flag. By default, the parameter is treated as an automation-enabled parameter.

### Normalization of Parameter Values

On the application side, parameters are always handled as unnormalized values. On the VST3 host side,
each parameter is always handled as a value normalized to the range 0.0–1.0. For Enum parameters, the user application handles them as integer values, such as Saw=0, Square=1, Triangle=2, and Sine=3. On the VST3 host side, these are handled as normalized values, such as Saw=0.0, Square=0.333..., Triangle=0.666..., and Sine=1.0.
Value conversion is performed at the parameter management layer of the plugin wrapper, so user code does not need to handle normalized values. Since values are handled as double-precision floating-point numbers during parameter exchange, the actual data passed are such as Saw=0.0, Square=1.0, Triangle=2.0, and Sine=3.0.
For Boolean parameters, false is treated as 0.0 and true as 1.0.
