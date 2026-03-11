# project(wrapper-vst3)
set(NAME wrapper-vst3)

add_library(${NAME} STATIC)

set_target_properties(${NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "wrapper-vst3"
    POSITION_INDEPENDENT_CODE ON
)

target_sources(${NAME} PRIVATE
  vst3-entry.cpp
  ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.cpp
  ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm
)

set_source_files_properties(
  ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")


target_link_libraries(${NAME} PRIVATE glaze::glaze sdk)
