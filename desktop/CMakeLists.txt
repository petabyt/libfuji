cmake_minimum_required(VERSION 3.10)

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
        ci.c
        win.res
    )
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LUA REQUIRED lua-5.3)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    pkg_check_modules(LIBXML REQUIRED libxml-2.0)
    set(PLATFORM_SOURCES unix.c ci.c)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/ ${CMAKE_CURRENT_BINARY_DIR}/lib)

set(SOURCES
    main.c jank.c backend.c
    ${PLATFORM_SOURCES}
    ../lib/camlib/src/lua/lua.c
    ../lib/camlib/src/lua/lua-cjson/lua_cjson.c ../lib/camlib/src/lua/lua-cjson/strbuf.c
)

if(WIN32)
    add_executable(fudge.exe ${SOURCES})
    target_link_libraries(fudge.exe ${WIN_LIBS} ${LDFLAGS} ${LIBXML_LIBRARIES})
elseif(UNIX)
    add_executable(fudge ${SOURCES})
    target_compile_options(libfudge
        PRIVATE -g -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations -Wstrict-aliasing=3
    )
    target_link_libraries(fudge m libfudge ${LIBUSB_LIBRARIES} ${LIBXML_LIBRARIES} ${LUA_LIBRARIES})
    target_include_directories(fudge PRIVATE ${LIBUSB_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS} ${LIBXML_INCLUDE_DIRS})
endif()
