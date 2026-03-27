#include <math.h>
#include <sonic/api/synthesizer-base.h>

namespace project1 {

enum OscWaveType {
  Saw = 0,
  Square,
  Triangle,
  Sine,
};

class Project1Synthesizer : public sonic::SynthesizerBase {
public:
  Project1Synthesizer() {}
  ~Project1Synthesizer() override {}

  void setupParameters(sonic::ParameterBuilder &builder) override;
  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) override;

  void setParameter(uint32_t id, double value) override;
  void noteOn(int noteNumber, double velocity) override;
  void noteOff(int noteNumber) override;
  void processAudio(float *bufferL, float *bufferR, uint32_t frames) override;

  void getDesiredEditorSize(uint32_t &width, uint32_t &height) override;
  std::string getEditorPageUrl() override;

private:
  float sampleRate = 0.f;
  int noteNumber = 60;
  bool gateOn = false;

  bool oscEnabled = true;
  OscWaveType oscWave = OscWaveType::Saw;
  float oscPitch = 0.5f;
  float oscVolume = 0.5f;

  float phase = 0.f;
};

} // namespace project1
