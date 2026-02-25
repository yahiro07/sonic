cmake_minimum_required(VERSION 3.14)
project(vst3wf)


add_library(vst3wf STATIC
  logger.cpp 
  logic/parameter_builder_impl.cpp
  modules/edit_controller_parameter_change_notifier.cpp
  modules/parameters_manager.cpp
  modules/webview_editor_view.cpp
)

target_link_libraries(vst3wf PRIVATE sdk)
target_include_directories(vst3wf PUBLIC .)

set_target_properties(vst3wf PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)
target_link_libraries(vst3wf PRIVATE glaze::glaze)


if(SMTG_MAC)
    target_sources(vst3wf PRIVATE
        general/mac_web_view.mm
    )

    target_link_libraries(vst3wf PRIVATE
        "-framework Cocoa"
        "-framework Webkit"
    )
endif()