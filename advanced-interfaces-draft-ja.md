# AdvancedSynthesizerBase

## 概要

これは現在計画中の発展的な機能を持ったシンセサイザーのベースインターフェイスです。
インターフェイスだけ考えています。これを満たすラッパー実装はまだありません(実装中です)

以下の機能の追加を考えています
- プラグインで内部的に利用するバイナリパラメータ
- 複数バージョン間でのパラメータのマイグレーション
- 音源側で持っている任意のバイト配列のデータをUIから取得する経路

これにより、MSEG対応のLFO/EGの実装や、波形モニター,スペクトラムアナライザーなどのプラグイン実装に対応できるようにします。

```cpp

enum ParameterFlags : int {
  None = 0,
  IsReadOnly = 1,
  IsHidden = 2,
  NonAutomatable = 4,
  // ローカルパラメータ, ホスト側には公開しない, Stateやプリセットには保存される
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
  // 可変長のバイナリデータのパラメータ, MSEGのポイント配列などを想定
  // ホスト側には公開されず内部的に管理する
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
  // diと配列サイズを指定してテレメトリデータのバッファを規定する, idの範囲は0~31
  // 画面に表示する波形データやFFTのバッファなどを音源側からUIに送信するために使用する
  // このサイズのバッファがフレームワーク側で確保され、ポーリングで使用される
  virtual void defineFloatArray(int id, uint32_t count) = 0;
};

class AdvancedSynthesizer: public SynthesizerBase {
public:
  virtual ~AdvancedSynthesizer() = default;
  //declared in base class
  // virtual void setupParameters(ParameterBuilder &builder) = 0;
  // virtual void prepareProcessing(double sampleRate, uint32_t maxFrameCount) = 0;

  // ローレベル寄りのprocessAudioExを使うかどうかのフラグ
  // 有効にした場合、波形生成処理ではprocessAudioExが呼ばれ、
  // precessAudio/setParameter/noteOn/noteOffは呼ばれなくなる
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

  // バイナリパラメータの適用
  virtual void setBinaryParameter(uint64_t address, const uint8_t *dataBytes,
                                  int dataLength) {}

  // ホストの演奏状態やテンポが変わったときに呼ばれるメソッド
  virtual void setHostPlayState(bool playing, double bpm) {}

  // ユーザー定義のカスタムアクションを受け取るメソッド
  // オーディオスレッドで呼ばれる想定, UI側から受けたメッセージの形式によってC++側で呼ばれるoverloadが変わる
  virtual void handleCustomAction(int id, double param1, double param2) {}
  virtual void handleCustomAction(int id, uint8_t *dataBytes, int dataLength) {}

  //UIから送った文字列を直接受け取る, UIスレッド上で受信する
  // virtual void __underConsideration__handleDirectMessageFromUi(std::string msg) {}
  // 音源側の単一値を取得するメソッド
  // virtual double __underConsideration__pullSingleDataExposed(int id) {}

  //テレメトリデータ定義の構築
  virtual void setupTelemetries(TelemetryBuilder &builder) {};
  //テレメトリデータを取得するメソッド, オーディオスレッド内で呼び出される
  //UI側からポーリングして音源側のバイト列データを取得しWebViewにデータを送信する
  virtual bool readTelemetry(int id, float *buffer, uint32_t count) {}

  // パラメータマイグレーション
  //現在の最新のパラメータのバージョンを固定値(ハードコード)で返す
  virtual int getParametersVersionLatest() { return 0; }
  // プリセットやstateが読み込まれたときに呼び出される
  // 音源の波形生成処理ではここでセットされた値を参照して処理を分岐して古いプリセットの互換性を維持する想定
  virtual void setParametersVersion(int parametersVersion) {}
  // プリセットやstateが読み込まれたときに呼び出される。読み出し時にパラメータセット自体を書き換える場合ここで処理する
  // リニアパラメータのマッピングの変更や、Enumパラメーターへの値の追加時の対応などを行う想定
  virtual void migrateParameters(std::map<uint64_t, double> &parameters,
                                 int parametersVersion) {}

  // デフォルトのデータ永続化実装を上書きするハンドラ
  virtual bool isOverridingStatePersistence() { return false; }
  virtual void readStateOverride(BinaryStream &stream) {}
  virtual void writeStateOverride(BinaryStream &stream) {}

  //declared in base class
  // virtual std::string getEditorPageUrl() = 0;
}
```
