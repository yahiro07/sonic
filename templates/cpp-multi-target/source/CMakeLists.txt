set(NAME plugin-source)
project(${NAME} LANGUAGES CXX)

add_library(${NAME} STATIC)

if(XCODE)
  set_target_properties(
    ${NAME}
    PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR
               "${CMAKE_BINARY_DIR}/lib/$(CONFIGURATION)-$(PLATFORM_NAME)"
               XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS
               "macosx iphonesimulator iphoneos"
               XCODE_ATTRIBUTE_SDKROOT auto)
endif()

target_sources(${NAME} PRIVATE project1-synthesizer.cpp project1-factory.cpp)

target_link_libraries(${NAME} PRIVATE sonic-headers plugin-view-webview)
