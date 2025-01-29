cmake_minimum_required(VERSION 2.4.4...3.5.0)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/camlib ${CMAKE_CURRENT_BINARY_DIR}/camlib)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fp ${CMAKE_CURRENT_BINARY_DIR}/fp)

project(libfudge)

set(FUDGE_CORE
    fuji.c
    tester.c
    data.c
    net.c
    discovery.c
    exif.c
    fuji_usb.c
    object.c
    fuji_lua.c
    lua_runtime.c
)

add_library(libfudge STATIC ${FUDGE_CORE})
target_compile_options(libfudge
    PRIVATE -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations
)
if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LUA REQUIRED lua-5.3)
endif()

target_include_directories(libfudge PUBLIC ${PROJECT_SOURCE_DIR} ${LUA_INCLUDE_DIRS})
target_link_libraries(libfudge PRIVATE libcamlib fp ${LUA_LIBRARIES})
target_compile_definitions(libfudge PRIVATE ${LUA_CFLAGS_OTHER})
set_target_properties(libfudge PROPERTIES OUTPUT_NAME libfudge)
