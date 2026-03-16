project(wrapper-auv3 LANGUAGES CXX OBJCXX)

add_library(wrapper-auv3 STATIC)

target_sources(
  wrapper-auv3
  PRIVATE ../../common/logger.cpp ./support/parameter-tree-wrapper.mm
          ./wrapper-auv3-root.mm)

target_compile_options(
  wrapper-auv3 PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
                       $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-weak>)

target_include_directories(
  wrapper-auv3
  PUBLIC ${CMAKE_CURRENT_LIST_DIR}
  PRIVATE ${PLUGIN_DIR})

set_target_properties(
  wrapper-auv3
  PROPERTIES PUBLIC_HEADER wrapper-auv3-root.h
             XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
             XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_WEAK YES)

# target_link_libraries( wrapper-auv3 PUBLIC plugin-view-webview "-framework
# AudioToolbox" "-framework AVFoundation" "-framework CoreAudioKit")
