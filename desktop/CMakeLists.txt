cmake_minimum_required(VERSION 3.10)

option(FUDGE_GET_HELLO_IMGUI "Download and compile using hello_imgui" ON)
project(fudge)

set(FUDGE ../lib)
set(CAMLIB_DIR ../lib/camlib)

if(WIN32)
    set(PLATFORM_SOURCES
        win.c
        win.rc
    )
    set(PLATFORM_LIBS
        -luser32 -lkernel32 -lgdi32 -lcomctl32 -luxtheme -lmsimg32 -lcomdlg32 -ld2d1 -ldwrite -lole32 -loleaut32 -loleacc -lssp -lurlmon -luuid -lws2_32 -static-libgcc -static-libstdc++ -lpthread
    )

    if(EXISTS "${CMAKE_SOURCE_DIR}/../../libwpd")
        add_subdirectory(../../libwpd/ ${CMAKE_CURRENT_BINARY_DIR}/libwpd)
    else()
        include(FetchContent)
        FetchContent_Declare(libwpd GIT_REPOSITORY https://github.com/petabyt/libwpd.git GIT_TAG master GIT_SHALLOW 1)
        FetchContent_MakeAvailable(libwpd)
    endif()
    list(APPEND PLATFORM_LIBS wpd_static)

    set(CMAKE_EXE_LINKER_FLAGS "-static -s")
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    set(PLATFORM_SOURCES unix.c ci.c)
endif()

if(FUDGE_GET_HELLO_IMGUI)
    set(HELLOIMGUI_BUILD_AUTOMATION_TEST OFF)
    set(HELLOIMGUI_ADD_APP_WITH_INSTALL OFF)
    set(HELLOIMGUI_USE_FREETYPE_PLUTOSVG OFF)
    set(HELLOIMGUI_USE_SDL2 OFF)
    set(HELLOIMGUI_USE_GLFW3 ON)
    set(HELLOIMGUI_HAS_VULKAN OFF)
    if(EXISTS "${CMAKE_SOURCE_DIR}/../../hello_imgui")
        add_subdirectory(../../hello_imgui/ ${CMAKE_CURRENT_BINARY_DIR}/hello_imgui)
        set(PLATFORM_SOURCES ${PLATFORM_SOURCES} hello_imgui.cpp)
    else()
        include(FetchContent)
        FetchContent_Declare(hello_imgui GIT_REPOSITORY https://github.com/pthom/hello_imgui.git GIT_TAG master GIT_SHALLOW 1)
        FetchContent_MakeAvailable(hello_imgui)
    endif()
    set(IMGUI_LIBRARIES hello-imgui::hello_imgui)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/ ${CMAKE_CURRENT_BINARY_DIR}/lib)

set(CAMLIB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/camlib/src)
set(SOURCES
    main.c jank.c backend.c ui.c im.cpp
    ${PLATFORM_SOURCES}
    ${CAMLIB_SOURCE_DIR}/lua/lua.c
    ${CAMLIB_SOURCE_DIR}/lua/lua-cjson/lua_cjson.c
    ${CAMLIB_SOURCE_DIR}/lua/lua-cjson/strbuf.c
)

add_executable(fudge ${SOURCES})
target_compile_options(fudge PRIVATE
    $<$<COMPILE_LANGUAGE:C>:
        -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=implicit-function-declaration -Werror=incompatible-pointer-types -Werror=return-type -Werror=deprecated-declarations -Wstrict-aliasing=3
    >
)
target_link_libraries(fudge m libfudge ${PLATFORM_LIBS} ${LIBUSB_LIBRARIES} ${LIBXML2_LIBRARIES} ${LUA_LIBRARIES} ${IMGUI_LIBRARIES})
target_include_directories(fudge PRIVATE ${LIBUSB_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIRS} ${IMGUI_INCLUDE_DIRS})
