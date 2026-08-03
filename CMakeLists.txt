# cmake -G Ninja -B build -DLIBFUJI_INCLUDE_CLI=ON && cmake --build build
cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

project(libfuji)
option(LIBFUJI_SHARED "Build using shared libraries" OFF)
option(LIBFUJI_INCLUDE_CLI "Include cli program for CI/testing" OFF)
option(LIBFUJI_MODULE "Compile as libpak module" OFF)

set(FUJI_CORE
    lib/fuji.c
    lib/tester.c
    lib/data.c
    lib/discovery.c
    lib/fuji_usb.c
    lib/plat.c
    lib/exif.c
)

if(LIBFUJI_MODULE)
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND AND NOT ANDROID)
        list(APPEND CMAKE_PREFIX_PATH "/usr/local")
        pkg_check_modules(LIBPAK IMPORTED_TARGET GLOBAL libpak)
        if(LIBPAK_FOUND)
            add_library(pak ALIAS PkgConfig::LIBPAK)
        endif()
    endif()
    set(FUJI_CORE ${FUJI_CORE} lib/module.c lib/bluetooth.c)
endif()

if(NOT TARGET fp)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fp ${CMAKE_CURRENT_BINARY_DIR}/fp)
endif()

if(NOT TARGET libpict)
    option(PTP_DEFAULT_LOGGING "Include default log/panic/error implementation" OFF)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libpict ${CMAKE_CURRENT_BINARY_DIR}/libpict)
endif()

if(LIBFUJI_SHARED)
    add_library(fuji SHARED ${FUJI_CORE})
else()
    add_library(fuji STATIC ${FUJI_CORE})
endif()
target_compile_options(fuji
    PRIVATE -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations
)

target_include_directories(fuji PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(fuji PUBLIC libpict fp)
if(TARGET pakrt)
    target_link_libraries(fuji PUBLIC pakrt)
endif()
if(TARGET pak)
    target_link_libraries(fuji PUBLIC pak)
endif()

if(LIBFUJI_INCLUDE_CLI)
    add_executable(libfuji-cli
        lib/cli.c
        lib/jank.c
    )
    target_link_libraries(libfuji-cli PUBLIC fuji)
endif()
