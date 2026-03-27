# SynthesizerBase

## 概要

以下では、プラグインのコアとなる`SynthesizerBase`クラスについて説明します。
新しいプラグインを作るときに、このクラスを継承して実装を行います。

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

`setParameter`, `noteOn`, `noteOff`, `processAudio`がDSP処理に関わる部分で、オーディオスレッド上で呼び出されます。その他のメソッドはメインスレッド上で呼ばれます。メインスレッドでのパラメタ管理はユーザーコードには含まれず、ラッパー側で管理されます。

### setupParameters

```cpp
virtual void setupParameters(ParameterBuilder &builder) = 0;
```

ホストでプラグインが読み込まれると、パラメーター定義のセットを構築するために`setupParameters`が呼び出されます。プラグインのパラメーター定義は`ParameterBuilder`のインスタンスを介して行われます。

#### note: コントローラの構成について

VST3ではプロセッサとコントローラが別のクラスとして実装されます。
sonicのVST3のラッパー実装ではプロセッサとコントローラで独立に`SynthesizerBase`のインスタンスを作り、それぞれ`setupParameters`を呼び出す構造になっています。`setupParameters`の実装は副作用のない、パラメタ定義を行うだけの処理にしてください。

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
  builder.addUnary(kOscPitch, "oscPitch", "OSC Pitch", 0.5);
  builder.addUnary(kOscVolume, "oscVolume", "OSC Volume", 0.5);
}
```

これはいくつかのパラメータを定義する例です。詳細については[パラメーターの定義の詳細](#パラメータの定義の詳細)のセクションを参照してください。

### prepareProcessing

```cpp
virtual void prepareProcessing(double sampleRate, uint32_t maxFrameCount) = 0;
```

実際のオーディオフレーム処理が始まる前に、`prepareProcessing`メソッドが呼ばれます。サンプリングレートと最大バッファサイズが渡されます。プラグインの処理内部で使用するバッファがある場合にはここで確保を行います。`prepareProcessing`メソッドは環境によって複数回呼ばれることがあります。

### setParameter

```cpp
virtual void setParameter(uint32_t id, double value) = 0;
```

パラメータが変更されると呼び出されます。`setupParameters`で登録したパラメタIDによって識別されます。`value`の値は非正規化された値です。

sonicフレームワークで常にラッパーからDSP処理の実装側に一方向にパラメータが送られる仕様になっています。ホストからのパラメタ要求にはラッパー側で持っている値を返しています。

### noteOn

```cpp
virtual void noteOn(int noteNumber, double velocity) = 0;
```

ホストからノートオンのイベントが送られてきたときに呼ばれます。noteNumberはMIDIの0~127の範囲の値です。velocityは0.0~1.0の浮動小数の値です。

### noteOff

```cpp
virtual void noteOff(int noteNumber) = 0;
```

ホストからノートオフのイベントが送られてきたときに呼ばれます。

### processAudio

```cpp
virtual void processAudio(float *bufferL, float *bufferR, uint32_t frames) = 0;
```

実際のオーディオフレームの処理を行うメソッドです。`bufferL`と`bufferR`はそれぞれ左右のチャンネルのオーディオフレームを表します。`frames`で指定されたサンプル数分の処理を行ってください。
プラグインの種類がinstrumentの場合呼び出し元でバッファが0で埋められて処理が呼ばれます。プラグインの種類がエフェクトの場合は入力された波形がそのまま入っていて、processAudioメソッドでこれを置き換えて出力する想定です。

#### note: 処理ブロックとサンプル単位でのイベント発行について

ラッパー側でホストから受けたサンプル単位のイベントの境界でフレームを区切って処理を行っています。`noteOn`, `noteOff`, `setParameter`はホストが指定するサンプルオフセットのタイミングで呼ばれます。そのため`processAudio`メソッドでの処理サンプル数(`frames`の値)は呼び出しごとに毎回変わります。1サンプルだけでの波形処理のような呼び出しが行われる可能性があり、これに対応できるように実装してください。

### getDesiredEditorSize

```cpp
virtual void getDesiredEditorSize(uint32_t &width, uint32_t &height) = 0;
```

UIを最初に表示するときに表示したい幅と高さを返します。widthとheightに希望するサイズを格納してください。

### getEditorPageUrl

```cpp
virtual std::string getEditorPageUrl() = 0;
```

WebViewで開かれるエディタのトップページのURLを返します。

#### リソースからの表示

```
app://www-bundles/index.html
app://www-bundles/index.html?debug=1
app://www-vanilla/index.html
app://www-vanilla/index.html?debug=1
```

`app://`から始まるURLはラッパーフレームワークが提供しているカスタムスキームのURLです。プラグインがアセットとして持っているコンテンツのファイルにマッピングされます。

`pages/www-bundles`にはviteなどのモジュールバンドラでビルドしたフロントエンドのファイルが配置されます。
`pages/www-vanilla`にはバンドラを使わないhtml/js/cssを直接配置格納するために使われます。
`app://`ではホスト部分が`pages`フォルダのサブフォルダに対応するように読み込みを行います。

URLの末尾に`debug=1`のフラグをつけるとフロントエンド側のデバッグログが有効になります。

#### 開発中の表示

```
http://localhost:3000
http://localhost:3000?debug=1
```

viteなどのバンドラのdev serverで表示しているURLにアクセスして開発を行う想定です。
これはローカルホストに限らず`https://example.com`のような任意のURLを指定して表示できます。

## パラメータの定義の詳細

プラグインのパラメータセットの構築は`ParameterBuilder`クラスのインスタンスを介して行われます。
`setupParameters`メソッドでこのビルダーが渡されるので、メソッドを呼び出してパラメタの登録を行ってください。

```cpp
class ParameterBuilder {
protected:
  using Str = std::string_view;
  using StrVec = const std::vector<std::string_view> &;

public:
  virtual ~ParameterBuilder() = default;
  virtual void addUnary(uint32_t id, Str paramKey, Str label,
                        double defaultValue, Str group = "",
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
//呼び出し例(再掲)
void Project1Synthesizer::setupParameters(ParameterBuilder &builder) {
  builder.addBool(kOscEnabled, "oscEnabled", "Osc Enabled", true);
  builder.addEnum(kOscWave, "oscWave", "Wave Type", "Saw",
                  {"Saw", "Square", "Triangle", "Sine"});
  builder.addUnary(kOscPitch, "oscPitch", "OSC Pitch", 0.5);
  builder.addUnary(kOscVolume, "oscVolume", "OSC Volume", 0.5);
```

各パラメータはID、キー、ラベル、デフォルト値、グループ、フラグを持っています。

### パラメータの種類

#### Unary Parameter

0.0~1.0の範囲の実数の連続値を持つパラメータです。

#### Enum Parameter

複数の選択肢の中から一つを選ぶようなパラメータです。オシレータの波形の種類やフィルタの種類(LFP/BPF/HPF)などを定義する際に使用します。

#### Bool Parameter

ON/OFFのような2値のパラメータです。

### id

パラメタの識別に使用されます。ホスト側に公開されます。
パラメタの`id`はuint32_tの型で表されます。VST3をターゲットとする場合、有効な`id`値の範囲は0<=`id`<=0x7FFFFFFFとなります。0x80000000以上の値は予約されており、使用できないので注意してください。`id`は0,1,2,3...のように連番である必要はなく、有効な範囲内の値であれば自由に設定できます。

### paramKey

プラグイン本体とWebViewのUIの間でパラメタを送る際に識別に使われる文字列です。
また、本体側でパラメータを永続化する際のキーとしても使われます。
このキーは、プラグインのバージョンの更新などでも変更を行わず、固定の値としてください。

### label

ホスト側で表示されるパラメータの表示名です。

### default value

パラメタの初期値です。パラメータの種類によって型が異なります。

### group

パラメータセットをグループ化する際のキーです。このキーが同じパラメータがホストのUI上でグループとして扱われます。実装はホスト側に依存します。

### flags

```cpp
enum ParameterFlags : int {
  None = 0,
  IsReadOnly = 1 << 0,
  IsHidden = 1 << 1,
  NonAutomatable = 1 << 2,
};
```

フラグには`None`, `IsReadOnly`, `IsHidden`, `NonAutomatable`の4つの値があります。これらをビットORで合成して指定します。パラメタはデフォルトでオートメーション可能なパラメタとして扱われます。

### パラメータの値の正規化について

アプリケーション側ではパラメータを常に非正規化された値で扱います。VST3のホスト側では
各パラメータは常に0.0~1.0の範囲に正規化した値で扱われます。Enumのパラメータの場合、ユーザーアプリケーション側では例えばSaw=0,Square=1,Triangle=2,Sine=3のような整数値で扱います。これがVST3のホスト側ではSaw=0.0,Square=0.333...,Triangle=0.666...,Sine=1.0のように正規化された値で扱われます。
プラグインラッパーのパラメータ管理のレイヤで値の変換を行っており、ユーザーコード側では正規化値を扱う必要がないようにしています。パラメータのやり取りで値はdouble型で扱われるため、受け渡しの際にはSaw=0.0,Square=1.0,Triangle=2.0,Sine=3.0のような実数値でのやりとりになります。
Boolean形式のパラメータは、falseが0.0, trueが1.0として扱われます。
