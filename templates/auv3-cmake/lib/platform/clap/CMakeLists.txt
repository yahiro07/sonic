project(wrapper-clap)

add_library(${PROJECT_NAME} STATIC 
  clap-entry-wrapper.cpp
  clap-rootage.cpp
  entry-controller.cpp
  ../../common/mac-web-view.mm
  ../../core/parameter-builder-impl.cpp
  ../../core/parameter-registry.cpp
  ../../domain/webview-bridge.cpp

)

set_source_files_properties(../../common/mac-web-view.mm PROPERTIES
  COMPILE_FLAGS "-fobjc-arc")

target_link_libraries(${PROJECT_NAME} PRIVATE
  "-framework Cocoa"
  "-framework WebKit"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "wrapper-clap"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(${PROJECT_NAME} PRIVATE glaze::glaze)


  


