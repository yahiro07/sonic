set(NAME project1-vst3)
add_library(${NAME} MODULE version.h plugin-factory.cpp)

if(APPLE)
  target_sources(
    ${NAME} PRIVATE ${VST3_SDK_ROOT}/public.sdk/source/main/macmain.cpp
                    ${VST3_SDK_ROOT}/public.sdk/source/main/moduleinit.cpp)
endif()

target_link_libraries(${NAME} PRIVATE plugin-source platform-vst3)

set_target_properties(
  ${NAME}
  PROPERTIES PREFIX ""
             OUTPUT_NAME "project1-vst3"
             BUNDLE True
             BUNDLE_EXTENSION vst3)

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

  add_custom_command(
    TARGET ${NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${NAME}>/../Resources"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PLUGIN_WWW_DIR}"
            "$<TARGET_FILE_DIR:${NAME}>/../Resources/www"
    VERBATIM)
endif()
