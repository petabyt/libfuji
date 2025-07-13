cmake_minimum_required(VERSION 2.4.4...3.5.0)

project(libfudge)
option(LIBFUDGE_IMPORT_LUA "Include Lua 5.3 from lua.cmake" ON)
option(LIBFUDGE_IMPORT_FP "Include libfp from subrepo" ON)

set(FUDGE_CORE
    fuji.c
    tester.c
    data.c
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

if(LIBFUDGE_IMPORT_LUA)
    include(../third_party/lua.cmake)
endif()

if(LIBFUDGE_IMPORT_FP)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fp ${CMAKE_CURRENT_BINARY_DIR}/fp)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libpict ${CMAKE_CURRENT_BINARY_DIR}/libpict)

target_include_directories(libfudge PUBLIC ${PROJECT_SOURCE_DIR} ${LUA_INCLUDE_DIRS})
target_link_libraries(libfudge PUBLIC libpict fp lua ${LUA_LIBRARIES})
target_compile_definitions(libfudge PUBLIC ${LUA_CFLAGS_OTHER})
set_target_properties(libfudge PROPERTIES OUTPUT_NAME libfudge)
