set(NAME platform-clap)

add_library(${NAME} STATIC)

target_sources(
  ${NAME} PRIVATE ../../common/logger.cpp ../../core/persistence.cpp
                  clap-entry-wrapper.cpp entry-controller.cpp)

target_link_libraries(${NAME} PUBLIC clap sonic-headers)

target_link_libraries(${NAME} PRIVATE glaze::glaze)
