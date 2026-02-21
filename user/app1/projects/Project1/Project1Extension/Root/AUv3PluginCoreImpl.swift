import SwiftUI

class AUv3PluginCoreImpl: AUv3PluginCore {
  let synthInstanceHandle: SynthInstanceHandle = createSynthesizerInstance()
  let parameterSpecProvider: ParameterSpecProvider = ParameterSpecProviderImpl()
  func createView(
    _ viewAccessibleResources: ViewAccessibleResources
  ) -> AnyView {
    return AnyView(MainContentView(viewAccessibleResources))
  }
}
