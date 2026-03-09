namespace pseudo_framework_design {
  //----------
  //📁core

  type Int32 = number;
  type Int64 = number;
  type Float = number;

  interface IPlatformControllerOutgoingParameterPort {
    beginEdit(id: Int32): void;
    performEdit(id: Int32, value: Float): void;
    endEdit(id: Int32): void;
  }

  interface IPlatformControllerIncomingParameterPort {
    subscribeHostOriginatedParameterChange(
      fn: (id: Int32, value: Float) => void,
    ): void;
    unsubscribeHostOriginatedParameterChange(): void;
  }

  enum ParameterEntryFlags {
    canAutomate = 1 << 0,
    isReadonly = 1 << 1,
    isHidden = 1 << 2,
  }

  type ParameterEntry = {
    type: "float" | "int" | "bool";
    id: Int32;
    name: string;
    defaultValue: Float;
    minValue: Float;
    maxValue: Float;
    flags: ParameterEntryFlags;
    units?: string;
    step?: Int32;
  };

  interface IProcessingData {
    events: (
      | {
          type: "noteOn";
          noteNumber: Int32;
          velocity: Float;
          sampleOffset: Int32;
        }
      | { type: "noteOff"; noteNumber: Int32; sampleOffset: Int32 }
      | {
          type: "parameterChange";
          id: Int32;
          value: Float;
          sampleOffset: Int32;
        }
    )[];
  }

  interface IWebViewIo {
    sendMessage(msg: string): void;
    subscribeMessage(fn: (msg: string) => void): void;
    unsubscribeMessage(): void;
  }

  interface ISynthesizerBase {
    getParameterEntries(): ParameterEntry[];
    processAudio(
      bufferL: Float[],
      bufferR: Float[],
      data: IProcessingData,
    ): void;
  }

  declare var d: any;
  type AnyTypeOf = any;

  interface IMainThreadParameterManager {
    setup(entries: ParameterEntry[]): void;
    setParameter(id: Int32, value: Float, notify: boolean): void;
    getAllParameters(): Record<Int32, Float>;
    subscribe(fn: (id: Int32, value: Float) => void): Int32;
    unsubscribe(token: Int32): void;
  }

  interface ParametersStore {
    get(id: Int32): Float;
    set(id: Int32, value: Float): void;
  }

  class VectorParameterStore implements ParametersStore {
    private parameters: Float[] = [];
    setCapacity(capacity: Int32) {
      this.parameters.length = capacity;
    }
    get(id: Int32): Float {
      return this.parameters[id];
    }
    set(id: Int32, value: Float): void {
      this.parameters[id] = value;
    }
  }
  class UnorderedMapParameterStore implements ParametersStore {
    private parameters: Record<Int32, Float> = {};
    get(id: Int32): Float {
      return this.parameters[id];
    }
    set(id: Int32, value: Float): void {
      this.parameters[id] = value;
    }
  }

  class MainThreadParameterManager {
    parameterIds: Int32[] = [];
    parameterStore: ParametersStore = new VectorParameterStore();
    listeners: Record<Int32, (id: Int32, value: Float) => void> = {};
    nextToken: Int32 = 0;

    setup(entries: ParameterEntry[]): void {
      const maxId = Math.max(...entries.map((e) => e.id));
      if (maxId + 1 > entries.length * 2) {
        this.parameterStore = new UnorderedMapParameterStore();
      } else {
        const parameterStore = new VectorParameterStore();
        parameterStore.setCapacity(maxId + 1);
        this.parameterStore = parameterStore;
      }
      entries.forEach((entry) => {
        this.parameterStore.set(entry.id, entry.defaultValue);
      });
      this.parameterIds = entries.map((e) => e.id);
    }

    setParameter(id: Int32, value: Float, notify: boolean): void {
      this.parameterStore.set(id, value);
      if (notify) {
        Object.values(this.listeners).forEach((fn) => fn(id, value));
      }
    }
    getParameter(id: Int32): Float {
      return this.parameterStore.get(id);
    }
    getAllParameters(): Record<Int32, Float> {
      const allParams: Record<Int32, Float> = {};
      for (const id of this.parameterIds) {
        allParams[id] = this.parameterStore.get(id);
      }
      return allParams;
    }
    subscribe(fn: (id: Int32, value: Float) => void): Int32 {
      const token = this.nextToken++;
      this.listeners[token] = fn;
      return token;
    }
    unsubscribe(token: Int32): void {
      delete this.listeners[token];
    }
  }

  class WebViewBridge {
    webViewIo: IWebViewIo | null = null;
    parameterSubscriptionToken: Int32 | null = null;
    constructor(
      private parameterOutputPort: IPlatformControllerOutgoingParameterPort,
      private parameterManager: IMainThreadParameterManager,
      private options: { updateInternalParameterOnPerformEdit: boolean },
    ) {}
    setup(webViewIo: IWebViewIo) {
      webViewIo.subscribeMessage((msg) => {
        let e = d.decodeMessage(msg);
        if (e.type === "uiLoaded") {
          const allParams = this.parameterManager.getAllParameters();
          webViewIo.sendMessage(d.json(allParams));
        } else if (e.type === "beginEdit") {
          this.parameterOutputPort.beginEdit(e.id);
        } else if (e.type === "performEdit") {
          this.parameterOutputPort.performEdit(e.id, e.value);
          if (this.options?.updateInternalParameterOnPerformEdit) {
            this.parameterManager.setParameter(e.id, e.value, false);
          }
        } else if (e.type === "endEdit") {
          this.parameterOutputPort.endEdit(e.id);
        }
      });
      this.parameterSubscriptionToken = this.parameterManager.subscribe(
        (id, value) => {
          webViewIo.sendMessage(d.json({ type: "setParameter", id, value }));
        },
      );
      this.webViewIo = webViewIo;
    }
    cleanup() {
      if (this.webViewIo) {
        this.webViewIo.unsubscribeMessage();
        if (this.parameterSubscriptionToken !== null) {
          this.parameterManager.unsubscribe(this.parameterSubscriptionToken);
          this.parameterSubscriptionToken = null;
        }
        this.webViewIo = null;
      }
    }
  }

  class DomainController {
    parameterManager: IMainThreadParameterManager;
    constructor(
      parameterEntries: ParameterEntry[],
      private parameterInputPort: IPlatformControllerIncomingParameterPort,
      private parameterOutputPort: IPlatformControllerOutgoingParameterPort,
    ) {
      this.parameterManager = new MainThreadParameterManager();
      this.parameterManager.setup(parameterEntries);
    }

    initialize() {
      this.parameterInputPort.subscribeHostOriginatedParameterChange(
        (id, value) => {
          this.parameterManager.setParameter(id, value, true);
        },
      );
    }

    terminate() {
      this.parameterInputPort.unsubscribeHostOriginatedParameterChange();
    }

    createWebViewBridge() {
      return new WebViewBridge(
        this.parameterOutputPort,
        this.parameterManager,
        {
          updateInternalParameterOnPerformEdit: true,
        },
      );
    }
  }

  interface ISynthesizerBase {
    getParameterEntries(): ParameterEntry[];
    processAudio(
      bufferL: Float[],
      bufferR: Float[],
      data: IProcessingData,
    ): void;
    setParameter(id: Int32, value: Float): void;
  }

  //----------
  //📁plugin

  enum ParameterID {
    gain = 0,
  }
  class MySynthesizer implements ISynthesizerBase {
    parameters = {
      gain: 0,
    } as { gain: number };

    getParameterEntries(): ParameterEntry[] {
      return [
        {
          type: "float",
          id: 0,
          name: "Gain",
          defaultValue: 0.5,
          minValue: 0,
          maxValue: 1,
          flags: ParameterEntryFlags.canAutomate,
        },
      ];
    }
    processAudio(
      bufferL: Float[],
      bufferR: Float[],
      data: IProcessingData,
    ): void {
      //for simplicity, we process parameter changes at the beginning of the block
      for (const event of data.events) {
        if (event.type === "parameterChange") {
          this.setParameter(event.id, event.value);
        }
      }
      const gain = this.parameters.gain;
      for (let i = 0; i < bufferL.length; i++) {
        bufferL[i] *= gain;
        bufferR[i] *= gain;
      }
    }
    setParameter(id: Int32, value: Float): void {
      if (id === ParameterID.gain) {
        this.parameters.gain = value;
      }
    }
  }
  const createSynthesizerInstance = () => new MySynthesizer();

  //----------
  //📁adapters/clap
  declare var d1: any;

  interface SPSCQueue<T, Size> {
    push(item: T): void;
    pop(): T | null;
  }

  class ClapParametersOutputPort implements IPlatformControllerOutgoingParameterPort {
    host: AnyTypeOf["clap_host_t"];
    hostParams: AnyTypeOf["clap_host_params_t"];

    setHosts(
      host: AnyTypeOf["clap_host_t"],
      hostParams: AnyTypeOf["clap_host_params_t"],
    ) {
      this.host = host;
      this.hostParams = hostParams;
    }

    outputEventQueue: SPSCQueue<
      | { type: "performEdit"; id: Int32; value: Float }
      | { type: "beginEdit" | "endEdit"; id: Int32 },
      256
    > = d1.createSPSCQueue();

    beginEdit(id: Int32): void {
      this.outputEventQueue.push({ type: "beginEdit", id });
      this.hostParams.request_flush(this.host);
    }
    performEdit(id: Int32, value: Float): void {
      this.outputEventQueue.push({ type: "performEdit", id, value });
      this.hostParams.request_flush(this.host);
    }
    endEdit(id: Int32): void {
      this.outputEventQueue.push({ type: "endEdit", id });
      this.hostParams.request_flush(this.host);
    }

    popOutputEvent():
      | { type: "performEdit"; id: Int32; value: Float }
      | { type: "beginEdit" | "endEdit"; id: Int32 }
      | null {
      return this.outputEventQueue.pop();
    }
  }

  class ClapParametersInputPort implements IPlatformControllerIncomingParameterPort {
    host: AnyTypeOf["clap_host_t"];

    setHosts(host: AnyTypeOf["clap_host_t"]) {
      this.host = host;
    }

    listener: ((id: Int32, value: Float) => void) | null = null;

    inputEventQueue: SPSCQueue<
      { type: "parameter"; id: Int32; value: Float },
      256
    > = d1.createSPSCQueue();

    mainThreadRequestedFlag: {
      //atomic
      value: boolean;
      store(newValue: boolean, mo: "memory_order_release"): void;
      compareExchange(
        expected: boolean,
        newValue: boolean,
        mo: "memory_order_acq_rel",
      ): boolean;
    } = { value: false } as any;

    subscribeHostOriginatedParameterChange(
      fn: (id: Int32, value: Float) => void,
    ): void {
      this.listener = fn;
    }
    unsubscribeHostOriginatedParameterChange(): void {
      this.listener = null;
    }

    pushInputEventFromAudioThread(id: Int32, value: Float) {
      this.inputEventQueue.push({ type: "parameter", id, value });
      if (
        this.mainThreadRequestedFlag.compareExchange(
          false,
          true,
          "memory_order_acq_rel",
        )
      ) {
        this.host.request_callback(this.host);
      }
    }

    mainThreadCallback() {
      this.mainThreadRequestedFlag.store(false, "memory_order_release");
      let e;
      while ((e = this.inputEventQueue.pop()) !== null) {
        if (e.type === "parameter" && this.listener) {
          this.listener(e.id, e.value);
        }
      }
    }
  }

  class EntryController {
    plugin: AnyTypeOf["clap_plugin_t"];
    host: AnyTypeOf["clap_host_t"];
    hostParams: AnyTypeOf["clap_host_params_t"];

    synth: ISynthesizerBase;
    domainController: DomainController;
    parameterInputPort: ClapParametersInputPort;
    parameterOutputPort: ClapParametersOutputPort;
    initialParameterQueue: SPSCQueue<{ id: Int32; value: Float }, 1024> =
      d1.createSPSCQueue();
    constructor() {
      this.synth = createSynthesizerInstance();
      const parameterEntries = this.synth.getParameterEntries();
      this.parameterInputPort = new ClapParametersInputPort();
      this.parameterOutputPort = new ClapParametersOutputPort();
      this.domainController = new DomainController(
        parameterEntries,
        this.parameterInputPort,
        this.parameterOutputPort,
      );
      this.domainController.initialize();
      for (const entry of parameterEntries) {
        this.initialParameterQueue.push({
          id: entry.id,
          value: entry.defaultValue,
        });
      }
    }

    initialize() {
      this.parameterInputPort.setHosts(this.host);
      this.parameterOutputPort.setHosts(this.host, this.hostParams);
    }

    processInternal(
      processData: AnyTypeOf["clap_audio_process_t"] | null,
      inEvents: AnyTypeOf["clap_input_events_t"],
      outEvents: AnyTypeOf["clap_output_events_t"],
    ): void {
      const data: IProcessingData = {
        events: [],
      };

      {
        //apply initial parameters
        let e: { id: Int32; value: Float } | null;
        while ((e = this.initialParameterQueue.pop())) {
          if (e) {
            data.events.push({
              type: "parameterChange",
              id: e.id,
              value: e.value,
              sampleOffset: 0,
            });
          }
        }
      }

      //handle input events
      for (let i = 0; i < inEvents.size(inEvents); i++) {
        const e = inEvents.get(inEvents, i);
        if (e.type === "clap_event_note_on") {
          data.events.push({
            type: "noteOn",
            noteNumber: e.note_number,
            velocity: e.velocity,
            sampleOffset: e.sample_offset,
          });
        } else if (e.type === "clap_event_note_off") {
          data.events.push({
            type: "noteOff",
            noteNumber: e.note_number,
            sampleOffset: e.sample_offset,
          });
        } else if (e.type === "clap_event_param_value") {
          //parameter flow: Host --> DSP
          data.events.push({
            type: "parameterChange",
            id: e.param_id,
            value: e.value,
            sampleOffset: e.sample_offset,
          });
          //parameter flow: Host --> UI
          this.parameterInputPort.pushInputEventFromAudioThread(
            e.param_id,
            e.value,
          );
        }
      }

      {
        //flush output events to host
        let e;
        while ((e = this.parameterOutputPort.popOutputEvent()) !== null) {
          if (e.type === "beginEdit") {
            outEvents.try_push({ type: "beginEdit", id: e.id });
          } else if (e.type === "performEdit") {
            //parameter flow: UI --> Host
            outEvents.try_push({
              type: "performEdit",
              id: e.id,
              value: e.value,
            });
            //parameter flow: UI --> DSP
            data.events.push({
              type: "parameterChange",
              id: e.id,
              value: e.value,
              sampleOffset: 0,
            });
          } else if (e.type === "endEdit") {
            outEvents.try_push({ type: "endEdit", id: e.id });
          }
        }
      }

      if (processData) {
        data.events.sort((a, b) => a.sampleOffset - b.sampleOffset);

        this.synth.processAudio(
          processData.audio_inputs[0].data32f,
          processData.audio_outputs[0].data32f,
          data,
        );
      }
    }

    //flush may be called on the UI thread, but since CLAP's specification ensures that process and flush are never called simultaneously,
    // mutual exclusion for process/flush is not implemented here.

    process(processData: AnyTypeOf["clap_process_t"]) {
      this.processInternal(
        processData,
        processData.in_events,
        processData.out_events,
      );
    }
    flush(
      inEvents: AnyTypeOf["clap_input_events_t"],
      outEvents: AnyTypeOf["clap_output_events_t"],
    ) {
      this.processInternal(null, inEvents, outEvents);
    }
    onMainThread() {
      this.parameterInputPort.mainThreadCallback();
    }
  }

  //----------
  //📁adapters/auv3
  declare var d2: any;

  class Auv3UiOriginatedParameterDestinationPort implements IPlatformControllerOutgoingParameterPort {
    flatParameterTree: Record<Int32, AnyTypeOf["Auv3Parameter"]> = {};
    token: Int32 = 0;

    constructor(private parameterTree: AnyTypeOf["Auv3ParameterTree"]) {
      const flatten = (node: any) => {
        if (node.type === "parameter") {
          this.flatParameterTree[node.id] = node;
        } else if (node.type === "folder") {
          node.children.forEach(flatten);
        }
      };
      flatten(this.parameterTree);
    }

    beginEdit(address: Int32): void {
      const parameter = this.flatParameterTree[address];
      parameter.setValue({ evenType: ".began" });
    }
    performEdit(address: Int32, value: Float): void {
      //parameter flow: Host,DSP <-- UI
      //this triggers parameterTree.implementorValueObserver callback
      // which then pushes the change to DSP via SPSCQueue
      const parameter = this.flatParameterTree[address];
      parameter.setValue({ value, originator: this.token });
    }
    endEdit(address: Int32): void {
      const parameter = this.flatParameterTree[address];
      parameter.setValue({ evenType: ".ended" });
    }
  }

  class AudioUnit {
    synth: ISynthesizerBase;
    dspParameterQueue: SPSCQueue<{ id: Int32; value: Float }, 1024> =
      d1.createSPSCQueue();

    uiParameterDestPort: Auv3UiOriginatedParameterDestinationPort;
    parameterManager: MainThreadParameterManager;
    constructor() {
      this.synth = createSynthesizerInstance();
      const parameterEntries = this.synth.getParameterEntries();
      const parameterTree = d2.createAuv3ParameterTree(parameterEntries);

      this.uiParameterDestPort = new Auv3UiOriginatedParameterDestinationPort(
        parameterTree,
      );
      for (const entry of parameterEntries) {
        this.dspParameterQueue.push({
          id: entry.id,
          value: entry.defaultValue,
        });
      }
      this.parameterManager = new MainThreadParameterManager();
      this.parameterManager.setup(parameterEntries);

      parameterTree.implementorValueObserver((param: any, value: Float) => {
        //parameter flow: Host/UI --> DSP
        this.dspParameterQueue.push({ id: param.address, value });
        //parameter flow: Host/UI --> UI (including ui originated reflected value)
        //cannot determine whether the change is originated from Host or UI here.
        this.parameterManager.setParameter(param.address, value, true);
      });
      parameterTree.implementorValueProvider((param: any) => {
        return this.parameterManager.getParameter(param.address);
      });
    }
    internalRenderBlock() {
      return (buffers: any, sourceEvents: any) => {
        const data: IProcessingData = {
          events: [],
        };
        let e: { id: Int32; value: Float } | null;
        while ((e = this.dspParameterQueue.pop())) {
          data.events.push({
            type: "parameterChange",
            id: e.id,
            value: e.value,
            sampleOffset: 0,
          });
        }
        const inputEvents = sourceEvents.map((e: any) => {
          if (e.type === "noteOn") {
            const { noteNumber, velocity, sampleOffset } = e;
            return { type: "noteOn", noteNumber, velocity, sampleOffset };
          } else if (e.type === "noteOff") {
            const { noteNumber, sampleOffset } = e;
            return { type: "noteOff", noteNumber, sampleOffset };
          } else if (e.type === "parameterChange") {
            const { id, value, sampleOffset } = e;
            return { type: "parameterChange", id, value, sampleOffset };
          }
        });
        data.events.push(...inputEvents);
        this.synth.processAudio(buffers[0], buffers[1], data);
      };
    }

    createWebViewBridge() {
      return new WebViewBridge(
        this.uiParameterDestPort,
        this.parameterManager,
        {
          updateInternalParameterOnPerformEdit: false,
        },
      );
    }
  }
  class AudioUnitViewController {
    audioUnit: AudioUnit | null = null;
    rootView: AnyTypeOf["NSView"];
    webView: AnyTypeOf["WebView"] & IWebViewIo;
    webViewBridge: WebViewBridge | null = null;

    createAudioUnit() {
      const audioUnit = new AudioUnit();
      this.connectViewToAudioUnit(audioUnit);
      this.audioUnit = audioUnit;
      return audioUnit;
    }
    viewDidLoad() {
      if (this.audioUnit && !this.webView) {
        this.connectViewToAudioUnit(this.audioUnit);
      }
    }
    connectViewToAudioUnit(audioUnit: AudioUnit) {
      this.webView = d.createWebView();
      this.webView.setParent(this.rootView);
      this.webViewBridge = audioUnit.createWebViewBridge();
      this.webViewBridge.setup(this.webView);
    }
    disconnectViewFromAudioUnit() {
      this.webViewBridge?.cleanup();
      this.webViewBridge = null;
      this.webView.close();
      this.webView = null;
    }
  }
}
