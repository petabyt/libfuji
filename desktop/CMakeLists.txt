cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

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
    set(WIN_LIBS "-lstdc++ -lgcc -lpthread")
    set(LDFLAGS "-s -static")
    set(PLATFORM_SOURCES
        win.c
        win.rc
    )
    include(libxml2.cmake)
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LUA REQUIRED lua-5.3)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    pkg_check_modules(LIBXML2 REQUIRED libxml-2.0)
    pkg_check_modules(SDL REQUIRED sdl3)
    set(PLATFORM_SOURCES unix.c ci.c)
    if(NOT ANDROID)
        include(hello_imgui.cmake)
        set(IMGUI_LIBRARIES hello-imgui::hello_imgui)
    endif()
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/ ${CMAKE_CURRENT_BINARY_DIR}/lib)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/lua ${CMAKE_CURRENT_BINARY_DIR}/lua)

set(SOURCES
    main.c jank.c backend.c imgui.cpp
    ${PLATFORM_SOURCES}
    ${CIMGUI_VULKAN_IMPL}
    ${CIMGUI_IMPL_SDL3}
    ../lib/camlib/src/lua/lua.c
    ../lib/camlib/src/lua/lua-cjson/lua_cjson.c ../lib/camlib/src/lua/lua-cjson/strbuf.c
)

add_executable(fudge ${SOURCES})
target_link_libraries(fudge m stdc++ libfudge ${IMGUI_LIBRARIES} ${LIBUSB_LIBRARIES} ${LIBXML2_LIBRARIES} ${LUA_LIBRARIES} ${SDL_LIBRARIES} vulkan)
target_include_directories(fudge PRIVATE ${LIBUSB_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIRS} ${SDL_INCLUDE_DIRS})

set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -g -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations -Wstrict-aliasing=3"
)

