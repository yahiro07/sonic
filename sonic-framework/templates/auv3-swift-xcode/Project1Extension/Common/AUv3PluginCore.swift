import SwiftUI

typealias SynthInstanceHandle = UnsafeMutablePointer<SynthesizerBase>

protocol ParameterSpecProvider {
  var latestParametersVersion: Int { get }
  func migrateParametersIfNeeded(paramVer: Int, rawParameters: inout [String: Float])
  func setupParameters(_ synthInstanceHandle: SynthInstanceHandle) -> ParameterTreeSpec
}

struct ViewAccessibleResources {
  let parameterTree: AUParameterTree
  let audioUnitPresenter: AudioUnitPresenter
  let storageFileIO: StorageFileIO
  let parameterSpecProvider: ParameterSpecProvider
  let stateKvs: StateKvs
}

protocol AUv3PluginCore {
  var synthInstanceHandle: SynthInstanceHandle { get }
  var parameterSpecProvider: ParameterSpecProvider { get }

  func createView(
    _ viewAccessibleResources: ViewAccessibleResources
  ) -> AnyView
}
