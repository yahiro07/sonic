set(NAME wrapper-clap)

add_library(
  ${NAME} STATIC
  common/mac-web-view.mm
  core/parameter-builder-impl.cpp
  core/parameter-registry.cpp
  domain/webview-bridge.cpp
  platform/clap/clap-entry-wrapper.cpp
  platform/clap/clap-rootage.cpp
  platform/clap/entry-controller.cpp)

set_source_files_properties(../../common/mac-web-view.mm
                            PROPERTIES COMPILE_FLAGS "-fobjc-arc")

target_link_libraries(${NAME} PRIVATE "-framework Cocoa" "-framework WebKit"
                                      clap)

set_target_properties(
  ${NAME}
  PROPERTIES PREFIX ""
             OUTPUT_NAME "wrapper-clap"
             POSITION_INDEPENDENT_CODE ON)

target_link_libraries(${NAME} PRIVATE glaze::glaze)
