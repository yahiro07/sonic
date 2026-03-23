set(NAME wrapper-clap)

add_library(${NAME} STATIC)

# target_compile_features(${NAME} PRIVATE cxx_std_23)

target_sources(${NAME} PRIVATE clap-entry-wrapper.cpp clap-rootage.cpp
                               entry-controller.cpp)

target_link_libraries(${NAME} PUBLIC sonic-headers clap)
