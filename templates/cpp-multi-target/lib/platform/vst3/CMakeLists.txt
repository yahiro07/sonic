set(NAME sonic_vst)

add_library(${NAME} STATIC)

set_target_properties(
  ${NAME}
  PROPERTIES PREFIX ""
             OUTPUT_NAME "sonic_vst"
             POSITION_INDEPENDENT_CODE ON)

target_sources(
  ${NAME}
  PRIVATE ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.cpp
          ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm
          ../../common/logger.cpp
          ../../core/parameter-builder-impl.cpp
          ../../core/parameter-registry.cpp
          support/parameter_change_notifier.cpp
          support/processor_state_helper.cpp
          vst_entry/vst_entry_wrapper.cpp
          vst_basis/plugin_processor.cpp
          vst_basis/plugin_controller.cpp
          ../../common/mac-web-view.mm
          ../../domain/webview-bridge.cpp)

set_source_files_properties(
  ../../common/mac-web-view.mm
  ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm
  PROPERTIES COMPILE_FLAGS "-fobjc-arc")

target_link_libraries(${NAME} PRIVATE glaze::glaze sdk "-framework Cocoa"
                                      "-framework WebKit")
