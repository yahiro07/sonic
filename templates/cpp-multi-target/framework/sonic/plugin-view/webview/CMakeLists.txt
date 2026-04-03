project(plugin-view-webview LANGUAGES CXX OBJCXX)

set(NAME plugin-view-webview)

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

# target_compile_features(${NAME} PRIVATE cxx_std_23)

target_sources(${NAME} PRIVATE mac-web-view.mm webview-bridge.cpp
                               webview-editor.cpp)

set_source_files_properties(mac-web-view.mm PROPERTIES COMPILE_FLAGS
                                                       "-fobjc-arc")

if(XCODE)
  # Cocoa prevents build with iOS, so it added later in xcodeproj
  target_link_libraries(${NAME} PUBLIC "-framework WebKit")
else()
  target_link_libraries(${NAME} PUBLIC "-framework WebKit -framework Cocoa")
endif()

target_link_libraries(${NAME} PRIVATE glaze::glaze sonic-headers)
