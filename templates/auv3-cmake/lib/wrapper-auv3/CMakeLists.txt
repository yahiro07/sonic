project(wrapper-auv3
	LANGUAGES OBJCXX CXX
)

add_library(wrapper-auv3 STATIC)

target_sources(wrapper-auv3
	PRIVATE
    ../common/parameter_builder_impl.cpp
		./wrapper-auv3-root.mm
    ./mac_web_view.mm
)

target_compile_options(wrapper-auv3
	PRIVATE
		$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
		$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-weak>
)

target_include_directories(wrapper-auv3
	PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
	PRIVATE
		${PLUGIN_DIR}
)

set_target_properties(wrapper-auv3 PROPERTIES
	PUBLIC_HEADER wrapper-auv3-root.h
)

target_link_libraries(wrapper-auv3 PRIVATE glaze::glaze)
