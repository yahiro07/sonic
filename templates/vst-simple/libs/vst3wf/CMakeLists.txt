add_library(vst3wf STATIC
  general/logger.cpp 
  logic/parameter_builder_impl.cpp
  logic/parameter_definitions_provider.cpp
  modules/parameter_change_notifier.cpp
  modules/parameters_manager.cpp
  modules/webview_editor_view.cpp
  modules/processor_state_helper.cpp
  vst/project1_processor.h
  vst/project1_processor.cpp
  vst/project1_controller.h
  vst/project1_controller.cpp
  vst_entry/vst_entry_wrapper.cpp
)

target_link_libraries(vst3wf PUBLIC sdk)

# Allow consumers to include headers as: #include "vst3wf/..."
target_include_directories(vst3wf
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(vst3wf PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

include(FetchContent)
if (NOT TARGET glaze::glaze)
  FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG main
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(glaze)
endif()
target_link_libraries(vst3wf PRIVATE glaze::glaze)


if(SMTG_MAC)
  set_source_files_properties(general/mac_web_view.mm PROPERTIES LANGUAGE CXX)

  target_sources(vst3wf PRIVATE general/mac_web_view.mm)

    target_link_libraries(vst3wf PRIVATE
        "-framework Cocoa"
    "-framework WebKit"
    )
endif()