set(NAME wrapper-clap)

add_library(${NAME} STATIC)

target_sources(${NAME} PRIVATE ../../common/logger.cpp clap-entry-wrapper.cpp
                               clap-rootage.cpp entry-controller.cpp)

target_link_libraries(${NAME} PUBLIC clap sonic-headers)
