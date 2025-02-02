cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

include(lua.cmake)

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
    set(WIN_LIBS "-lstdc++ -lgcc -lpthread")
    set(LDFLAGS "-s -static")
    set(PLATFORM_SOURCES
        win.c
        win.rc
    )
    include(libxml2.cmake)
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    set(PLATFORM_SOURCES unix.c ci.c)
endif()

if(FUDGE_GET_HELLO_IMGUI AND NOT ANDROID)
    include(hello_imgui.cmake)
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

# import libxml2
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(LIBXML2_WITH_CATALOG "Add the Catalog support" OFF)
option(LIBXML2_WITH_DEBUG "Add the debugging module" OFF)
option(LIBXML2_WITH_HTML "Add the HTML support" OFF)
option(LIBXML2_WITH_HTTP "Add the HTTP support" OFF)
option(LIBXML2_WITH_ICONV "Add ICONV support" ON)
option(LIBXML2_WITH_ICU "Add ICU support" OFF)
option(LIBXML2_WITH_ISO8859X "Add ISO8859X support if no iconv" OFF)
option(LIBXML2_WITH_LEGACY "Add deprecated APIs for compatibility" OFF)
option(LIBXML2_WITH_LZMA "Use liblzma" OFF)
option(LIBXML2_WITH_MODULES "Add the dynamic modules support" OFF)
option(LIBXML2_WITH_OUTPUT "Add the serialization support" ON)
option(LIBXML2_WITH_PATTERN "Add the xmlPattern selection interface" OFF)
option(LIBXML2_WITH_PROGRAMS "Build programs" OFF)
option(LIBXML2_WITH_PUSH "Add the PUSH parser interfaces" OFF)
option(LIBXML2_WITH_PYTHON "Build Python bindings" OFF)
option(LIBXML2_WITH_READLINE "readline support for xmllint shell" OFF)
option(LIBXML2_WITH_REGEXPS "Add Regular Expressions support" OFF)
option(LIBXML2_WITH_SAX1 "Add the older SAX1 interface" OFF)
option(LIBXML2_WITH_TESTS "Build tests" OFF)
option(LIBXML2_WITH_THREADS "Add multithread support" OFF)
option(LIBXML2_WITH_TLS "Enable thread-local storage" OFF)
option(LIBXML2_WITH_VALID "Add the DTD validation support" OFF)
option(LIBXML2_WITH_XINCLUDE "Add the XInclude support" OFF)
option(LIBXML2_WITH_XPATH "Add the XPATH support" OFF)
option(LIBXML2_WITH_ZLIB "Use libz" OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libxml2 ${CMAKE_CURRENT_BINARY_DIR}/libxml2)
set(LIBXML2_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libxml2/include ${CMAKE_CURRENT_BINARY_DIR}/libxml2)
set(LIBXML2_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/libxml2/libxml2.a)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/ ${CMAKE_CURRENT_BINARY_DIR}/lib)

set(SOURCES
    main.c jank.c backend.c ui.cpp
    ${PLATFORM_SOURCES}
    ${CIMGUI_VULKAN_IMPL}
    ${CIMGUI_IMPL_SDL3}
    ../lib/camlib/src/lua/lua.c
    ../lib/camlib/src/lua/lua-cjson/lua_cjson.c
    ../lib/camlib/src/lua/lua-cjson/strbuf.c
)

add_executable(fudge ${SOURCES})
target_link_libraries(fudge m stdc++ libfudge ${LIBUSB_LIBRARIES} ${LIBXML2_LIBRARIES} ${LUA_LIBRARIES} ${IMGUI_LIBRARIES} ${GLFW3_LIBRARIES} ${VULKAN_LIBRARIES})
target_include_directories(fudge PRIVATE ${LIBUSB_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIRS} ${IMGUI_INCLUDE_DIRS})

set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -g -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations -Wstrict-aliasing=3"
)

