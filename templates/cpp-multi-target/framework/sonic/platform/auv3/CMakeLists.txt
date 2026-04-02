set(NAME platform-auv3)

project(${NAME} LANGUAGES CXX OBJCXX)

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

target_sources(
  ${NAME}
  PRIVATE ../../common/logger.cpp ../../common/udp-log-emitter.cpp
          ../../core/persistence.cpp ./support/parameter-tree-wrapper.mm
          ./wrapper-auv3-root.mm)

target_compile_options(
  ${NAME} PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
                  $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-weak>)

target_include_directories(
  ${NAME}
  PUBLIC ${CMAKE_CURRENT_LIST_DIR}
  PRIVATE ${PLUGIN_DIR})

set_target_properties(
  ${NAME}
  PROPERTIES PUBLIC_HEADER wrapper-auv3-root.h
             XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
             XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_WEAK YES)

target_link_libraries(${NAME} PRIVATE sonic-headers glaze::glaze)
