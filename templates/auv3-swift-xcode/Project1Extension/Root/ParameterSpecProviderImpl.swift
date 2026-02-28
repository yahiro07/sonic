import AudioToolbox
import Foundation

class ParameterSpecProviderImpl: ParameterSpecProvider {
  let latestParametersVersion: Int = 1

  func migrateParametersIfNeeded(paramVer: Int, rawParameters: inout [String: Float]) {
    //invoked when parameters are loaded, apply your own migration if needed
  }

  func setupParameters(_ synthInstanceHandle: SynthInstanceHandle) -> ParameterTreeSpec {
    return pullParameterDefinitionsFromCppSide(synthInstanceHandle)
    //if you want to make more flexible parameter tree than C++ side definition,
    //write parameters directly here instead of calling pullParameterDefinitionsFromCppSide().
  }
}
