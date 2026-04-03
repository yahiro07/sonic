import AVFoundation
import CoreAudioKit

public class AudioUnit: AUAudioUnit, @unchecked Sendable {
  let pluginCore: AUv3PluginCore = AUv3PluginCoreImpl()
  // C++ Objects
  var kernel = DSPKernel()
  var processHelper: AUProcessHelper?

  private var outputBus: AUAudioUnitBus?
  private var _outputBusses: AUAudioUnitBusArray!

  private var format: AVAudioFormat

  let parameterStore = ParameterStore()
  let stateKvs = StateKvs()
  private(set) var isHostedInStandaloneApp: Bool = false
  private(set) var currentPresetParametersVersion: Int = 0

  @objc override init(
    componentDescription: AudioComponentDescription, options: AudioComponentInstantiationOptions
  ) throws {
    logger.log("AudioUnit init")
    self.format = AVAudioFormat(standardFormatWithSampleRate: 44_100, channels: 2)!
    try super.init(componentDescription: componentDescription, options: options)
    outputBus = try AUAudioUnitBus(format: self.format)
    outputBus?.maximumChannelCount = 2
    _outputBusses = AUAudioUnitBusArray(
      audioUnit: self, busType: AUAudioUnitBusType.output, busses: [outputBus!])
    processHelper = AUProcessHelper(&kernel)
    kernel.setSynthesizerInstance(pluginCore.synthInstanceHandle)
  }

  public override func supportedViewConfigurations(
    _ availableViewConfigurations: [AUAudioUnitViewConfiguration]
  ) -> IndexSet {
    return IndexSet(integersIn: 0..<availableViewConfigurations.count)
  }

  func pullRTAudioPortalEventOne() -> DownstreamEvent? {
    var rawEvent = RealtimeHostEvent()
    guard processHelper?.popRealtimeHostEvent(&rawEvent) == true else {
      return nil
    }
    return mapRealtimeHostEventToDownstreamEvent(rawEvent)
  }

  //push MIDI note sent from internal UI
  func pushScheduledMidiEvent(_ bytes: [UInt8]) {
    guard let midiBlock = scheduleMIDIEventBlock else { return }
    midiBlock(
      AUEventSampleTimeImmediate, 0, Int(bytes.count), bytes
    )
  }

  public override var outputBusses: AUAudioUnitBusArray {
    return _outputBusses
  }

  public override var maximumFramesToRender: AUAudioFrameCount {
    get {
      return kernel.maximumFramesToRender()
    }

    set {
      kernel.setMaximumFramesToRender(newValue)
    }
  }

  public override var shouldBypassEffect: Bool {
    get {
      return kernel.isBypassed()
    }

    set {
      kernel.setBypass(newValue)
    }
  }

  // MARK: - MIDI
  public override var audioUnitMIDIProtocol: MIDIProtocolID {
    return processHelper!.AudioUnitMIDIProtocol()
  }

  // MARK: - Rendering
  public override var internalRenderBlock: AUInternalRenderBlock {
    return processHelper!.internalRenderBlock()
  }

  // Allocate resources required to render.
  // Subclassers should call the superclass implementation.
  public override func allocateRenderResources() throws {
    let outputChannelCount = self.outputBusses[0].format.channelCount

    kernel.setMusicalContextBlock(self.musicalContextBlock)
    kernel.initialize(Int32(outputChannelCount), outputBus!.format.sampleRate)

    processHelper?.setChannelCount(0, self.outputBusses[0].format.channelCount)

    try super.allocateRenderResources()
  }

  // Deallocate resources allocated in allocateRenderResourcesAndReturnError:
  // Subclassers should call the superclass implementation.
  public override func deallocateRenderResources() {

    // Deallocate your resources.
    kernel.deInitialize()

    super.deallocateRenderResources()
  }

  public func setupParameterTree() {
    let parameterSpecs =
      pluginCore.parameterSpecProvider.setupParameters(pluginCore.synthInstanceHandle)
    let parameterTree = parameterSpecs.createAUParameterTree()
    self.parameterTree = parameterTree

    let maxAddress = parameterTree.allParameters.map { $0.address }.max() ?? 0
    let capacity64 = maxAddress &+ 1
    let capacity = UInt32(min(capacity64, UInt64(UInt32.max)))
    parameterStore.setParameterCapacity(capacity)

    for param in parameterTree.allParameters {
      parameterStore.setParameter(param.address, param.value)
      kernel.setParameter(param.address, param.value)
    }

    setupParameterCallbacks()
  }

  private func setupParameterCallbacks() {
    parameterTree?.implementorValueObserver = { [weak self] param, value -> Void in
      self?.parameterStore.setParameter(param.address, value)
      self?.processHelper?.pushParameterChange(param.address, value)
    }
    parameterTree?.implementorValueProvider = { [weak self] param in
      return self?.parameterStore.getParameter(param.address) ?? 0.0
    }
    parameterTree?.implementorStringFromValueCallback = { param, valuePtr in
      guard let value = valuePtr?.pointee else {
        return "-"
      }
      return NSString.localizedStringWithFormat("%.f", value) as String
    }
  }

  func emitParametersState() -> (parametersVersion: Int, parameters: [String: Float]) {
    let parametersVersion = pluginCore.parameterSpecProvider.latestParametersVersion
    var rawParameters: [String: Float] = [:]
    parameterTree?.allParameters.forEach { param in
      rawParameters[param.identifier] = param.value
    }
    return (parametersVersion, rawParameters)
  }

  func applyParametersState(
    _ parametersVersion: Int, _ parameters: [String: Float]
  ) {
    var modParameters = parameters
    pluginCore.parameterSpecProvider.migrateParametersIfNeeded(
      paramVer: parametersVersion, rawParameters: &modParameters)
    parameterTree?.allParameters.forEach { param in
      if let value = parameters[param.identifier] {
        param.value = value
      }
    }
    self.currentPresetParametersVersion = parametersVersion
    kernel.setParametersVersion(Int32(parametersVersion))
  }

  public override var fullState: [String: Any]? {
    get {
      logger.info("fullState saving")
      let baseState = super.fullState
      var state: [String: Any] = [
        "type": componentDescription.componentType,
        "subtype": componentDescription.componentSubType,
        "manufacturer": componentDescription.componentManufacturer,
        "version": baseState?["version"] as? Int ?? 0,
      ]
      state["kvsItems"] = stateKvs.items
      let (parametersVersion, parameters) = emitParametersState()
      state["parametersVersion"] = parametersVersion
      state["parameters"] = parameters
      return state
    }

    set(newValue) {
      logger.info("fullState restoration")
      guard let state = newValue else { return }
      if let flag = state["MySynth1.hostedInStandaloneApp"] as? Bool {
        self.isHostedInStandaloneApp = flag
      }
      if let parametersVersion = state["parametersVersion"] as? Int,
        let parameters = state["parameters"] as? [String: Float]
      {
        applyParametersState(parametersVersion, parameters)
      }
      if let kvsItems = state["kvsItems"] as? [String: String] {
        stateKvs.setItems(kvsItems)
      }
      //skipping super.fullState to avoid overwriting our custom restoration results.
      // super.fullState = state
    }
  }

}
