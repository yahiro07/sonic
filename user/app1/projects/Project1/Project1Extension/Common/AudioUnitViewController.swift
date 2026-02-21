import Combine
import CoreAudioKit
import SwiftUI
import os

private let log = Logger(
  subsystem: "net.miqsel.synth2511.Project1Extension", category: "AudioUnitViewController")

@MainActor
public class AudioUnitViewController: AUViewController, AUAudioUnitFactory {
  let synthInstanceHandle: SynthInstanceHandle = createSynthesizerInstance()
  var audioUnit: AUAudioUnit?
  var hostingController: HostingController<MainView>?
  // private var observation: NSKeyValueObservation?

  private let audioUnitPresenter: AudioUnitPresenterImpl = AudioUnitPresenterImpl()
  private let storageFileIO: StorageFileIOImpl = StorageFileIOImpl()

  /* iOS View lifcycle
  public override func viewWillAppear(_ animated: Bool) {
  	super.viewWillAppear(animated)
  
  	// Recreate any view related resources here..
  }
  
  public override func viewDidDisappear(_ animated: Bool) {
  	super.viewDidDisappear(animated)
  
  	// Destroy any view related content here..
  }
  */

  /* macOS View lifcycle
  public override func viewWillAppear() {
  	super.viewWillAppear()
  
  	// Recreate any view related resources here..
  }
  
  public override func viewDidDisappear() {
  	super.viewDidDisappear()
  
  	// Destroy any view related content here..
  }
  */

  deinit {
  }

  public override func viewDidLoad() {
    logger.mark("viewDidLoad")
    super.viewDidLoad()
    guard let audioUnit = self.audioUnit else {
      return
    }
    configureSwiftUIView(audioUnit: audioUnit)
  }

  nonisolated public func createAudioUnit(with componentDescription: AudioComponentDescription)
    throws -> AUAudioUnit
  {
    return try DispatchQueue.main.sync {
      logger.mark("createAudioUnitInternal")
      logger.log("Loaded From: " + Bundle.main.bundlePath)
      try storageFileIO.debugLogDataLocation()

      audioUnit = try AudioUnit(
        componentDescription: componentDescription, options: [])

      guard let audioUnit = self.audioUnit as? AudioUnit else {
        log.error("Unable to create AudioUnit")
        return audioUnit!
      }

      audioUnit.setSynthInstanceHandle(synthInstanceHandle)
      audioUnitPresenter.setAudioUnit(audioUnit)

      defer {
        // Configure the SwiftUI view after creating the AU, instead of in viewDidLoad,
        // so that the parameter tree is set up before we build our @AUParameterUI properties
        DispatchQueue.main.async {
          self.configureSwiftUIView(audioUnit: audioUnit)
        }
      }

      let parameterSpecs = createProject1ExtensionParameterSpecs(synthInstanceHandle)
      let parameterTree = parameterSpecs.createAUParameterTree()
      audioUnit.setupParameterTree(parameterTree)

      // self.observation = audioUnit.observe(\.allParameterValues, options: [.new]) {
      //   object, change in
      //   guard let tree = audioUnit.parameterTree else { return }

      //   // This insures the Audio Unit gets initial values from the host.
      //   for param in tree.allParameters { param.value = param.value }
      // }

      guard audioUnit.parameterTree != nil else {
        log.error("Unable to access AU ParameterTree")
        return audioUnit
      }

      return audioUnit
    }
  }

  private func configureSwiftUIView(audioUnit: AUAudioUnit) {
    logger.mark("configureSwiftUIView")

    if let host = hostingController {
      host.removeFromParent()
      host.view.removeFromSuperview()
    }
    guard let audioUnit = self.audioUnit,
      // let parameterTree = audioUnit.parameterTree,
      let observableParameterTree = audioUnit.observableParameterTree
      // let pluginCore = audioUnit.pluginCore
    else {
      return
    }

    // let parameterMigrator = pluginCore.parametersMigrator
    // let viewAccessibleResources = ViewAccessibleResources(
    //   parameterTree: parameterTree, audioUnitPortal: self.audioUnitPortal,
    //   presetFilesIO: self.presetFilesIO, parametersMigrator: parameterMigrator,
    //   stateKvs: audioUnit.stateKvs)

    // let content = pluginCore.createView(viewAccessibleResources)
    let content = MainView(parameterTree: observableParameterTree)
    let host = HostingController(rootView: content)

    self.addChild(host)
    host.view.frame = self.view.bounds
    self.view.addSubview(host.view)
    hostingController = host

    // Make sure the SwiftUI view fills the full area provided by the view controller
    host.view.translatesAutoresizingMaskIntoConstraints = false
    host.view.topAnchor.constraint(equalTo: self.view.topAnchor).isActive = true
    host.view.leadingAnchor.constraint(equalTo: self.view.leadingAnchor).isActive = true
    host.view.trailingAnchor.constraint(equalTo: self.view.trailingAnchor).isActive = true
    host.view.bottomAnchor.constraint(equalTo: self.view.bottomAnchor).isActive = true
    self.view.bringSubviewToFront(host.view)
  }

  #if os(macOS)
    override public func viewDidAppear() {
      super.viewDidAppear()
      startEventLoop()
    }

    override public func viewWillDisappear() {
      super.viewWillDisappear()
      stopEventLoop()
    }
  #elseif os(iOS)
    override public func viewDidAppear(_ animated: Bool) {
      super.viewDidAppear(animated)
      startEventLoop()
    }

    override public func viewWillDisappear(_ animated: Bool) {
      super.viewWillDisappear(animated)
      stopEventLoop()
    }
  #endif

  private var eventTimer: Timer?
  private var cancellables = Set<AnyCancellable>()

  private func startEventLoop() {
    eventTimer = Timer(
      timeInterval: 1.0 / 60.0,
      repeats: true
    ) { [weak self] _ in
      self?.audioUnitPresenter.drainEventsOnMainThread(maxCount: 32)
    }
    RunLoop.main.add(eventTimer!, forMode: .common)
  }
  private func stopEventLoop() {
    eventTimer?.invalidate()
    eventTimer = nil
    cancellables.removeAll()
  }
}
