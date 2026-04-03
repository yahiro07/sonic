add_library(sonic STATIC
  general/logger.cpp 
  logic/parameter_builder_impl.cpp
  logic/parameter_definitions_provider.cpp
  modules/parameter_change_notifier.cpp
  modules/parameters_manager.cpp
  modules/webview_editor_view.cpp
  modules/processor_state_helper.cpp
  vst_entry/vst_entry_wrapper.cpp
  vst_basis/plugin_processor.cpp
  vst_basis/plugin_controller.cpp
)

target_link_libraries(sonic PUBLIC sdk)

# Allow consumers to include headers as: #include "sonic/..."
target_include_directories(sonic
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(sonic PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

include(FetchContent)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v7.1.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)

target_link_libraries(sonic PRIVATE glaze::glaze)


if(SMTG_MAC)
  set_source_files_properties(general/mac_web_view.mm PROPERTIES LANGUAGE CXX)

  target_sources(sonic PRIVATE general/mac_web_view.mm)

    target_link_libraries(sonic PRIVATE
        "-framework Cocoa"
    "-framework WebKit"
    )
endif()