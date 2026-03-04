project(sonic_common)

add_library(${PROJECT_NAME} STATIC
  general/logger.cpp 
  logic/parameter_builder_impl.cpp
  logic/parameter_definitions_provider.cpp
  general/mac_web_view.mm
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)
  
set_source_files_properties(general/mac_web_view.mm PROPERTIES LANGUAGE CXX)

target_link_libraries(${PROJECT_NAME} PRIVATE
  "-framework Cocoa"
  "-framework WebKit"
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
