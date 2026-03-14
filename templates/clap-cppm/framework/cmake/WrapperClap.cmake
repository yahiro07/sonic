set(NAME wrapper-clap)

add_library(${NAME} STATIC)

set_target_properties(
  ${NAME}
  PROPERTIES PREFIX ""
             OUTPUT_NAME "wrapper-clap"
             POSITION_INDEPENDENT_CODE ON
             CXX_EXTENSIONS OFF
             CXX_SCAN_FOR_MODULES ON)

target_compile_features(${NAME} PRIVATE cxx_std_20)

target_sources(
  ${NAME}
  PRIVATE common/mac-web-view.mm
          core/parameter-builder-impl.cpp
          core/parameter-registry.cpp
          domain/webview-bridge.cpp
          platform/clap/clap-entry-wrapper.cpp
          platform/clap/clap-rootage.cpp
          platform/clap/entry-controller.cpp)

target_sources(${NAME} PRIVATE FILE_SET CXX_MODULES FILES
                               platform/clap/my-module.cpp)

set_source_files_properties(common/mac-web-view.mm PROPERTIES COMPILE_FLAGS
                                                              "-fobjc-arc")

target_link_libraries(${NAME} PRIVATE "-framework Cocoa" "-framework WebKit"
                                      clap glaze::glaze)
