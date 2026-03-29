import Combine

@MainActor
class WebViewHub {
  private let flatParameterTree: FlatObservableParameters
  private let audioUnitPresenter: AudioUnitPresenter
  private let storageFileIO: StorageFileIO
  private let parameterSpecProvider: ParameterSpecProvider
  private let stateKvs: StateKvs

  private var webViewIo: WebViewIoProtocol?

  private let valueTracker: ObservableValueTracker = ObservableValueTracker()
  private var valueTrackerStarted = false
  private var portalSubscription: AnyCancellable?
  private var webViewIoSubscription: AnyCancellable?

  private var uiReady = false

  init(
    _ viewAccessibleResources: ViewAccessibleResources
  ) {
    logger.info("BasicWebViewHub init")
    self.flatParameterTree = FlatObservableParameters(
      parameterTree: ObservableAUParameterGroup(viewAccessibleResources.parameterTree))
    self.audioUnitPresenter = viewAccessibleResources.audioUnitPresenter
    self.storageFileIO = viewAccessibleResources.storageFileIO
    self.parameterSpecProvider = viewAccessibleResources.parameterSpecProvider
    self.stateKvs = viewAccessibleResources.stateKvs
  }

  deinit {
    portalSubscription?.cancel()
    webViewIoSubscription?.cancel()
  }

  private func sendMessageToUI(_ msg: MessageFromApp) {
    if !uiReady {
      logger.warn("Tried to send message before UI uiReady, skipping: \(msg)")
      return
    }
    if let jsDataDictionary = mapMessageFromApp_toDictionary(msg) {
      webViewIo?.sendRawMessageToUI(data: jsDataDictionary)
    } else {
      logger.warn("Failed to map message from app to dictionary: \(msg)")
    }
  }

  private func debugDumpCurrentParameters() {
    logger.log("Current Parameters in debugDumpCurrentParameters:")
    for (paramKey, paramEntry) in flatParameterTree.entries {
      logger.log("\(paramKey): \(paramEntry.value)")
    }
  }

  private func handleMessageFromUI(
    msg: MessageFromUI,
  ) {
    switch msg {
    case .putLogItem(let timestamp, let logKind, let message):
      loggerCore.pushLogItem(
        LogItem(timestamp: timestamp, subsystem: "ui", logKind: logKind, message: message))
    case .uiLoaded:
      logger.info("received UI loaded")
      uiReady = true
      if audioUnitPresenter.isHostedInStandaloneApp {
        sendMessageToUI(.standaloneAppFlag)
      }
      let latestParamVer = parameterSpecProvider.latestParametersVersion
      sendMessageToUI(.latestParametersVersion(version: latestParamVer))
      // self.debugDumpCurrentParameters()
      let params = flatParameterTree.entries.mapValues { $0.value }
      sendMessageToUI(.bulkSendParameters(params: params))
      startAUStateListeners()

    case .beginParameterEdit(let paramKey):
      if let paramEntry = flatParameterTree.entries[paramKey] {
        paramEntry.onEditingChanged(true)
        // logger.log("begin parameter edit: \(paramKey)")
      }
    case .endParameterEdit(let paramKey):
      if let paramEntry = flatParameterTree.entries[paramKey] {
        paramEntry.onEditingChanged(false)
        // logger.log("end parameter edit: \(paramKey)")
      }
    case .setParameter(let paramKey, let value):
      logger.log("received parameter changed from UI: \(paramKey) = \(value)")
      if let paramEntry = flatParameterTree.entries[paramKey] {
        valueTracker.reserveEchoSuppression(paramKey: paramKey, value: value)
        paramEntry.value = value
      } else {
        logger.warn("Unknown parameter key from UI: \(paramKey)")
      }
    case .loadFullParameters(let parametersVersion, let parameters):
      // logger.log("Received full parameters from UI: \(parameters)")
      audioUnitPresenter.applyParametersState(parametersVersion, parameters)
    case .noteOnRequest(let noteNumber):
      // logger.log("Note On Request from UI: \(noteNumber)")
      audioUnitPresenter.noteOnFromUI(noteNumber, velocity: 1.0)
    case .noteOffRequest(let noteNumber):
      // logger.log("Note Off Request from UI: \(noteNumber)")
      audioUnitPresenter.noteOffFromUI(noteNumber)
    //
    case .rpcReadFileRequest(let rpcId, let path, let skipIfNotExists):
      do {
        let content = try storageFileIO.readFile(path: path, skipIfNotExist: skipIfNotExists)
        sendMessageToUI(.rpcReadFileResponse(rpcId: rpcId, success: true, content: content))
      } catch {
        logger.error("RPC readFile error: \(error)")
        sendMessageToUI(.rpcReadFileResponse(rpcId: rpcId, success: false, content: ""))
      }
    case .rpcWriteFileRequest(let rpcId, let path, let content, let append):
      do {
        try storageFileIO.writeFile(path: path, content: content, append: append)
        sendMessageToUI(.rpcWriteFileResponse(rpcId: rpcId, success: true))
      } catch {
        logger.error("RPC writeFile error: \(error)")
        sendMessageToUI(.rpcWriteFileResponse(rpcId: rpcId, success: false))
      }
    case .rpcDeleteFileRequest(let rpcId, let path):
      do {
        try storageFileIO.deleteFile(path: path)
        sendMessageToUI(.rpcDeleteFileResponse(rpcId: rpcId, success: true))
      } catch {
        logger.error("RPC deleteFile error: \(error)")
        sendMessageToUI(.rpcDeleteFileResponse(rpcId: rpcId, success: false))
      }
    case .rpcLoadStateKvsItems(let rpcId):
      sendMessageToUI(.rpcLoadStateKvsItemsResponse(rpcId: rpcId, items: stateKvs.items))
    case .writeStateKvsItem(let key, let value):
      stateKvs.write(key, value)
    case .deleteStateKvsItem(let key):
      stateKvs.delete(key)
    }
  }

  func handleDownstreamEvent(_ event: DownstreamEvent) {
    switch event {
    case .hostNoteOn(let noteNumber, let velocity):
      // logger.log("Received Note On from host: \(noteNumber) velocity: \(velocity)")
      sendMessageToUI(.hostNoteOn(noteNumber: noteNumber, velocity: velocity))
    case .hostNoteOff(let noteNumber):
      // logger.log("Received Note Off from host: \(noteNumber)")
      sendMessageToUI(.hostNoteOff(noteNumber: noteNumber))
    case .hostPlayState(let playState):
      logger.log("Received Play State from host @whub: \(playState)")
    case .hostTempo(let tempo):
      logger.log("Received Tempo from host: \(tempo)")
    case .parametersVersionChanged(let parametersVersion):
      logger.log("Received parameters version changed event: \(parametersVersion)")
    }
  }

  func startAUStateListeners() {
    if !valueTrackerStarted {
      valueTracker.setReceiver { [weak self] key, value in
        // logger.log("PT value changed, send to UI, \(key) \(value)")
        self?.sendMessageToUI(.setParameter(paramKey: key, value: value))
      }
      for (paramKey, paramEntry) in flatParameterTree.entries {
        valueTracker.trackParameterValue(paramKey: paramKey, paramEntry: paramEntry)
      }
      valueTrackerStarted = true
    }

    portalSubscription?.cancel()
    portalSubscription = self.audioUnitPresenter.events.sink { event in
      self.handleDownstreamEvent(event)
    }
  }

  func bindWebViewIo(_ webViewIo: WebViewIoProtocol) {
    logger.info("bindWebViewIo")
    self.webViewIo = webViewIo

    webViewIoSubscription?.cancel()
    webViewIoSubscription = webViewIo.subscribeRawMessageFromUI { [weak self] jsDataDictionary in
      if let msg: MessageFromUI = mapMessageFromUI_fromDictionary(jsDataDictionary) {
        self?.handleMessageFromUI(msg: msg)
      } else {
        logger.warn("Unknown or invalid message from UI \(jsDataDictionary)")
      }
    }
  }
}
