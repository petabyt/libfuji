cmake_minimum_required(VERSION 3.10)

option(FUDGE_GET_HELLO_IMGUI "Download and compile using hello_imgui" ON)
option(FUDGE_USE_SYSTEM_IMGUI "Use imgui and glfw/vulkan from OS" OFF)
project(fudge)

set(FUDGE ../lib)
set(CAMLIB_DIR ../lib/camlib)
include_directories(
    ${FUDGE}
    ${CAMLIB_DIR}/src
    ${CAMLIB_DIR}/src/lua
    ${FUDGE}/fp/src
)

if(WIN32)
    set(PLATFORM_SOURCES
        win.c
        win.rc
    )
    set(PLATFORM_LIBS
        -luser32 -lkernel32 -lgdi32 -lcomctl32 -luxtheme -lmsimg32 -lcomdlg32 -ld2d1 -ldwrite -lole32 -loleaut32 -loleacc -lssp -lurlmon -luuid -lws2_32 -static-libgcc -static-libstdc++ -lpthread
    )
    set(CMAKE_EXE_LINKER_FLAGS "-static -s")
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    set(PLATFORM_SOURCES unix.c ci.c)
endif()

if(FUDGE_GET_HELLO_IMGUI)
    include(cmake/hello_imgui.cmake)
    set(IMGUI_LIBRARIES hello-imgui::hello_imgui)
    set(PLATFORM_SOURCES ${PLATFORM_SOURCES} hello_imgui.cpp)
endif()
if(FUDGE_USE_SYSTEM_IMGUI AND UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(IMGUI REQUIRED imgui)
    pkg_check_modules(GLFW3 REQUIRED glfw3)
    pkg_check_modules(VULKAN REQUIRED vulkan)
    set(PLATFORM_SOURCES ${PLATFORM_SOURCES} glfw_imgui.cpp)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/ ${CMAKE_CURRENT_BINARY_DIR}/lib)

set(CAMLIB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/camlib/src)
set(SOURCES
    main.c jank.c backend.c ui.cpp
    ${PLATFORM_SOURCES}
    ${CAMLIB_SOURCE_DIR}/lua/lua.c
    ${CAMLIB_SOURCE_DIR}/lua/lua-cjson/lua_cjson.c
    ${CAMLIB_SOURCE_DIR}/lua/lua-cjson/strbuf.c
)

set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -g -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations -Wstrict-aliasing=3"
)

add_executable(fudge ${SOURCES})
target_link_libraries(fudge m libfudge ${PLATFORM_LIBS} ${LIBUSB_LIBRARIES} ${LIBXML2_LIBRARIES} ${LUA_LIBRARIES} ${IMGUI_LIBRARIES} ${GLFW3_LIBRARIES} ${VULKAN_LIBRARIES})
target_include_directories(fudge PRIVATE ${LIBUSB_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIRS} ${IMGUI_INCLUDE_DIRS})
