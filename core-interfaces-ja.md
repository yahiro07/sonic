## SynthesizerBase

## 概要

以下では、プラグインのコアとなるSynthesizerBaseクラスについて説明します。
新しいプラグインを作るときに、このクラスを継承して実装を行います。

```cpp
class SynthesizerBase {
public:
  virtual ~SynthesizerBase() = default;
  virtual void setupParameters(ParameterBuilder &builder) = 0;
  virtual void prepare(double sampleRate, int32_t maxFrameCount) = 0;
  virtual void setParameter(uint32_t address, double value) = 0;
  virtual void noteOn(int32_t noteNumber, double velocity) = 0;
  virtual void noteOff(int32_t noteNumber) = 0;
  virtual void process(float *bufferL, float *bufferR, int32_t frames) = 0;
  virtual std::string getEditorPageUrl() = 0;
};
```

### setupParameters

```cpp
virtual void setupParameters(ParameterBuilder &builder) = 0;
```

ホストでプラグインが読み込まれると、パラメーター定義のセットを構築するためにsetupParametersが呼び出されます。プラグインのパラメーター定義はParameterBuilderのインスタンスを介して行われます。

note: VST3ではプロセッサとコントローラが別のクラスとして実装されます。
sonicのVST3のラッパー実装ではプロセッサとコントローラで独立にSynthesizerBaseのインスタンスを作り、それぞれsetupParametersを呼び出す構造になっています。setupParametersの実装は副作用のない、パラメタ定義を行うだけの処理にしてください。

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

これはいくつかのパラメータを定義する例です。詳細については[パラメーターの定義の詳細](#パラメーターの定義の詳細)のセクションを参照してください。

### prepare

```cpp
virtual void prepare(double sampleRate, int32_t maxFrameCount) = 0;
```

実際のオーディオフレーム処理が始まる前に、prepareメソッドが呼ばれます。サンプリングレートと最大バッファサイズが渡されます。プラグインの処理内部で使用するバッファがある場合にはここで確保を行います。prepareメソッドは環境によって複数回呼ばれることがあります。

### setParameter

```cpp
virtual void setParameter(uint32_t address, double value) = 0;
```

パラメータが変更されると呼び出されます。setupParametersで登録したパラメタアドレスによって識別されます。valueの値は非正規化された値です。この処理はオーディオスレッドで呼ばれます。

sonicフレームワークで常にラッパーからプラグイン本体の実装側に一方向にパラメータが送られる仕様になっています。ホストからのパラメタ要求にはラッパー側で持っている値を返しています。

### noteOn

```cpp
virtual void noteOn(int32_t noteNumber, double velocity) = 0;
```

ホストからノートオンのイベントが送られてきたときに呼ばれます。noteNumberはMIDIの0~127の範囲の値です。velocityは0~1の範囲の値です。

### noteOff

```cpp
virtual void noteOff(int32_t noteNumber) = 0;
```

ホストからノートオフのイベントが送られてきたときに呼ばれます。

### process

```cpp
virtual void process(float *bufferL, float *bufferR, int32_t frames) = 0;
```

実際のオーディオフレームの処理を行うメソッドです。bufferLとbufferRはそれぞれ左右のチャンネルのオーディオフレームを表します。framesで指定されたサンプル数分の処理を行ってください。
プラグインの種類がinstrumentの場合呼び出し元でバッファが0で埋められて処理が呼ばれます。プラグインの種類がエフェクトの場合は入力された波形がそのまま入っていて、processメソッドでこれを置き換えて出力する想定です。

note: 処理ブロックとサンプル単位でのイベント発行について

ラッパー側でホストから受けたサンプル単位のイベントの境界でフレームを区切って処理を行っています。noteOn, noteOff,setParameterはホストが指定するサンプルオフセットのタイミングで呼ばれます。そのためprocessメソッドでの処理サンプル数(framesの値)は呼び出しごとに毎回変わります。1サンプルだけでの波形処理のような呼び出しが行われる可能性があり、これに対応できるように実装してください。

### getEditorPageUrl

```cpp
virtual std::string getEditorPageUrl() = 0;
```

WebViewで開かれるエディタのトップページのURLを返します。

#### リソースからの表示

```
app://local/index.html
app://local/index.html?debug=1
```

app://localで始まるURLはラッパーフレームワークが提供しているカスタムスキームのURLです。プラグインがアセットとして持っているコンテンツのファイルにマッピングされます。
VST3のラッパー実装ではapp://local/ が resources/www ディレクトリに対応します。
URLの末尾にdebug=1のフラグをつけるとフロントエンド側のデバッグログが有効になります。

#### 開発中の表示

```
http://localhost:3000
http://localhost:3000?debug=1
```

viteなどのバンドラのdev serverで表示しているURLにアクセスして開発を行う想定です。
これはローカルホストに限らずhttps://example.comのような任意のURLを指定して表示できます。

## パラメーターの定義の詳細

プラグインのパラメタセットの構築は以下のbuilderクラスのインスタンスを介して行われます。
setupParametersメソッドでこのビルダーが渡されるので、メソッドを呼び出してパラメタの登録を行ってください。

```cpp
class ParameterBuilder {
protected:
  using Str = std::string_view;
  using StrVec = const std::vector<std::string> &;

public:
  virtual ~ParameterBuilder() = default;
  virtual void addUnary(uint32_t address, Str identifier, Str label,
                        double defaultValue, Str group = "",
                        ParameterFlags flags = ParameterFlags::None) = 0;
  virtual void addEnum(uint32_t address, Str identifier, Str label,
                       Str defaultValueString, StrVec valueStrings,
                       Str group = "",
                       ParameterFlags flags = ParameterFlags::None) = 0;
  virtual void addBool(uint32_t address, Str identifier, Str label,
                       bool defaultValue, Str group = "",
                       ParameterFlags flags = ParameterFlags::None) = 0;
};
```

各パラメータはアドレス、識別子、ラベル、デフォルト値、グループ、フラグを持っています。

### パラメータの種類

#### Unary Parameter

0~1の範囲の実数の連続値を持つパラメータです。

#### Enum Parameter

複数の選択肢の中から一つを選ぶようなパラメータです。オシレータの波形の種類やフィルタの種類(LFP/BPF/HPF)などを定義する際に使用します。

#### Bool Parameter

ON/OFFのような二値のパラメータです。

### address

アドレスはuint32_tの型で表されます。有効なアドレス値の範囲は0<=address<=0x7FFFFFFFです。0x80000000以上の値は予約されており、使用できないので注意してください。0,1,2,3...のように連番である必要はなく、各パラメータのアドレスは有効な範囲内の値であれば自由に設定できます。

### identifier

identifierは文字列で、パラメータを永続化する際の識別に使用されます。またプラグイン本体とWebViewのUIの間で値を送る際にはこのidentifierがキーとして使用されます。
プラグインラッパーの実装では,パラメタの整数のaddress値ではなく文字列のidentifierをキーにしてパラメタセットを保存/復元します。
このキーは、プラグインのバージョンの更新などでも変更を行わず、固定の値としてください。

### label

ホスト側で表示されるパラメタの表示名です。

### default value

パラメタの初期値です。パラメタの種類によって型が異なります。

### group

パラメタセットをグループ化する際のキーです。このキーが同じパラメタがホストのUI上でグループとして扱われます。実装はホスト側に依存します。

### パラメータの値の扱いについて

アプリケーション側ではパラメータを常に非正規化された値で扱います。VST3のホスト側では
各パラメータは常に0~1の範囲に正規化した値で扱われます。。Enumのパラメータの場合、ユーザーアプリケーション側では例えばSaw=0,Square=1,Triangle=2,Sine=3のような整数値で扱います。これがVST3のホスト側ではSaw=0.0,Square=0.333...,Triangle=0.666...,Sine=1.0のように正規化された値で扱われます。
プラグインラッパーのパラメタ管理のレイヤで値の変換を行っており、ユーザーコード側では正規化値を扱う必要がないようにしています。パラメタのやり取りで値はdouble型で扱われるため、受け渡しの際にはSaw=0.0,Square=1.0,Triangle=2.0,Sine=3.0のような実数値でのやりとりになります。
Boolean形式のパラメタは、falseが0.0, trueが1.0として扱われます。
