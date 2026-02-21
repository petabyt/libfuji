# cmake -G Ninja -B build -DLIBFUJI_INCLUDE_CLI=ON && cmake --build build
cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

project(libfuji)
option(LIBFUJI_INCLUDE_CLI "Include cli program for CI/testing" OFF)

set(FUJI_CORE
    lib/fuji.c
    lib/tester.c
    lib/data.c
    lib/discovery.c
    lib/exif.c
    lib/fuji_usb.c
    lib/object.c
    lib/plat.c
)
if(TARGET pak)
    set(FUJI_CORE ${FUJI_CORE} lib/module.c)
endif()

add_library(libfuji STATIC ${FUJI_CORE})
target_compile_options(libfuji
    PRIVATE -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations
)

if(NOT TARGET fp)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fp ${CMAKE_CURRENT_BINARY_DIR}/fp)
endif()

if(NOT TARGET libpict)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libpict ${CMAKE_CURRENT_BINARY_DIR}/libpict)
endif()

target_include_directories(libfuji PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(libfuji PUBLIC libpict fp)
if(TARGET pak)
    target_link_libraries(libfuji PUBLIC pak)
endif()
set_target_properties(libfuji PROPERTIES OUTPUT_NAME libfuji)

if(LIBFUJI_INCLUDE_CLI)
    add_executable(libfuji-cli
        lib/cli.c
        lib/jank.c
    )
    target_link_libraries(libfuji-cli PUBLIC libfuji)
endif()
