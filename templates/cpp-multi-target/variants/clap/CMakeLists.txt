set(NAME project1-clap)
add_library(${NAME} MODULE plugin-entry.cpp)

set_target_properties(
  ${NAME}
  PROPERTIES PREFIX ""
             OUTPUT_NAME "project1-clap"
             BUNDLE True
             BUNDLE_EXTENSION clap)

target_link_libraries(${NAME} PRIVATE plugin wrapper-clap)

if(APPLE)
  if(NOT PROJECT_VERSION)
    set(PROJECT_VERSION "1.0.0")
  endif()
  set_target_properties(
    ${NAME}
    PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER com.my-company.${NAME}
               MACOSX_BUNDLE_BUNDLE_NAME ${NAME}
               MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
               MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
               MACOSX_BUNDLE_INFO_PLIST
               ${CMAKE_SOURCE_DIR}/cmake/base-info.plist.in)
endif()
