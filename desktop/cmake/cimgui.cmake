include(ExternalProject)

ExternalProject_Add(cimgui.external
	GIT_REPOSITORY https://github.com/cimgui/cimgui.git
	GIT_SHALLOW 1
	GIT_TAG docking_inter
	CMAKE_ARGS
		-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}
		-DIMGUI_STATIC=ON
	SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/cimgui.external/src/
	BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/cimgui.external/bin/
	BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/cimgui.external/bin/cimgui.a ${CMAKE_CURRENT_BINARY_DIR}/cimgui.external/src/imgui/backends/imgui_impl_vulkan.cpp ${CMAKE_CURRENT_BINARY_DIR}/cimgui.external/src/imgui/backends/imgui_impl_sdl3.cpp
	INSTALL_COMMAND ""
)

ExternalProject_Get_Property(cimgui.external BINARY_DIR SOURCE_DIR)

set(CIMGUI_VULKAN_IMPL ${SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp)
set(CIMGUI_IMPL_SDL3 ${SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp)

add_library(cimgui STATIC IMPORTED)
set_target_properties(cimgui PROPERTIES
	IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/cimgui.external/bin/cimgui.a
	INTERFACE_INCLUDE_DIRECTORIES "${SOURCE_DIR}"
)

add_dependencies(cimgui cimgui.external)

set(CIMGUI_INCLUDE_DIRS ${SOURCE_DIR} ${SOURCE_DIR}/imgui ${SOURCE_DIR}/imgui/backends)
set(CIMGUI_LIBRARIES cimgui)
