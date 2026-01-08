cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

project(libfudge)
option(LIBFUDGE_IMPORT_FP "Include libfp from subrepo" ON)
option(LIBFUDGE_INCLUDE_CLI "Include cli program for CI/testing" OFF)

set(FUDGE_CORE
    fuji.c
    tester.c
    data.c
    discovery.c
    exif.c
    fuji_usb.c
    object.c
    plat.c
)

add_library(libfudge STATIC ${FUDGE_CORE})
target_compile_options(libfudge
    PRIVATE -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations
)

if(LIBFUDGE_IMPORT_FP)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fp ${CMAKE_CURRENT_BINARY_DIR}/fp)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libpict ${CMAKE_CURRENT_BINARY_DIR}/libpict)

target_include_directories(libfudge PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(libfudge PUBLIC libpict fp)
set_target_properties(libfudge PROPERTIES OUTPUT_NAME libfudge)

if(LIBFUDGE_INCLUDE_CLI)
    add_executable(libfudge-cli
        cli.c
        jank.c
    )
    target_link_libraries(libfudge-cli PUBLIC libfudge)
endif()
