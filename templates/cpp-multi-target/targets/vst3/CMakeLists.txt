project(project1-vst3)

add_library(${PROJECT_NAME} MODULE main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "project1-vst3"
    BUNDLE True
    BUNDLE_EXTENSION vst3
)

target_link_libraries(${PROJECT_NAME} PRIVATE plugin wrapper-vst3)

if (APPLE)
    if (NOT PROJECT_VERSION)
        set(PROJECT_VERSION "1.0.0")
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER com.my-company.${PROJECT_NAME}
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()
