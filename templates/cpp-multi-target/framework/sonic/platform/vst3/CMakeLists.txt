set(NAME platform-vst3)

add_library(${NAME} STATIC)

target_sources(
  ${NAME}
  PRIVATE ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.cpp
          ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm
          ../../common/logger.cpp
          support/parameter_change_notifier.cpp
          support/processor_state_helper.cpp
          vst_entry/vst_entry_wrapper.cpp
          vst_basis/plugin_processor.cpp
          vst_basis/plugin_controller.cpp)

set_source_files_properties(
  ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm
  PROPERTIES COMPILE_FLAGS "-fobjc-arc")

target_link_libraries(${NAME} PRIVATE glaze::glaze)

target_link_libraries(${NAME} PUBLIC sdk sonic-headers)
