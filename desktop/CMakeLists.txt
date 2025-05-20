cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(fudge)

# Import local libwpd so libpict won't download it
if(WIN32)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../libwpd")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../libwpd/ ${CMAKE_CURRENT_BINARY_DIR}/libwpd)
    endif()
endif()

# libfudge
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/ ${CMAKE_CURRENT_BINARY_DIR}/lib)

# hello_imgui
set(HELLOIMGUI_BUILD_AUTOMATION_TEST OFF)
set(HELLOIMGUI_ADD_APP_WITH_INSTALL OFF)
option(HELLOIMGUI_USE_FREETYPE "" OFF)
option(HELLOIMGUI_USE_FREETYPE_PLUTOSVG "" OFF)
set(HELLOIMGUI_USE_SDL2 OFF)
set(HELLOIMGUI_USE_GLFW3 ON)
set(HELLOIMGUI_HAS_VULKAN OFF)
if(EXISTS "${CMAKE_SOURCE_DIR}/../../hello_imgui")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../hello_imgui/ ${CMAKE_CURRENT_BINARY_DIR}/hello_imgui)
else()
    include(FetchContent)
    FetchContent_Declare(hello_imgui GIT_REPOSITORY https://github.com/pthom/hello_imgui.git GIT_TAG master GIT_SHALLOW 1)
    FetchContent_MakeAvailable(hello_imgui)
endif()

if(WIN32)
    set(PLATFORM_SOURCES win.c win.rc)
    set(PLATFORM_LIBS -luser32 -lkernel32 -lgdi32 -lcomctl32 -luxtheme -lmsimg32 -lcomdlg32 -ld2d1 -ldwrite -lole32 -loleaut32 -loleacc -lssp -lurlmon -luuid -lws2_32 -static-libgcc -static-libstdc++ -lpthread)
elseif(UNIX)
    set(PLATFORM_SOURCES unix.c ci.c)
endif()

set(LIBPICT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/libpict/src)
add_executable(fudge
    hello_imgui.cpp
    main.c jank.c backend.c ui.c im.cpp
    ${PLATFORM_SOURCES}
    ${LIBPICT_SOURCE_DIR}/lua/lua.c
    ${LIBPICT_SOURCE_DIR}/lua/lua-cjson/lua_cjson.c
    ${LIBPICT_SOURCE_DIR}/lua/lua-cjson/strbuf.c
)
target_compile_options(fudge PRIVATE
    $<$<COMPILE_LANGUAGE:C>:
        -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=implicit-function-declaration -Werror=incompatible-pointer-types -Werror=return-type -Werror=deprecated-declarations
    >
)
target_link_libraries(fudge m libfudge
    hello-imgui::hello_imgui
    $<$<BOOL:${WIN32}>:-static -s>
    ${PLATFORM_LIBS}
)
target_compile_definitions(fudge PUBLIC
    $<$<CONFIG:Debug>:
        FUDGE_DEBUG=1
    >
)
target_include_directories(fudge PRIVATE ${LIBUSB_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIRS})
install(TARGETS fudge)

if(FUDGE_CREATE_APPIMAGE)
    function(create_appimage)
        cmake_parse_arguments(PARSE_ARGV 0 ARG "" "TARGET;ICON;DESKTOP" "")

        install(TARGETS ${ARG_TARGET} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/AppDir/usr/bin)

        find_program(LINUXDEPLOY linuxdeploy)
        if(NOT LINUXDEPLOY)
            message(FATAL_ERROR "linuxdeploy not found")
        endif()
        find_program(APPIMAGETOOL appimagetool)
        if(NOT APPIMAGETOOL)
            message(FATAL_ERROR "appimagetool not found")
        endif()

        # Strip binary included in linuxdeploy is broken, disable stripping
        add_custom_target(${ARG_TARGET}_AppImage ALL
            COMMAND rm -rf AppDir
            COMMAND NO_STRIP=true linuxdeploy --appdir=${CMAKE_CURRENT_BINARY_DIR}/AppDir --executable=$<TARGET_FILE:${ARG_TARGET}> -d ${ARG_DESKTOP} -i ${ARG_ICON} --output appimage
            COMMAND appimagetool ${CMAKE_CURRENT_BINARY_DIR}/AppDir
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${ARG_TARGET}
        )
    endfunction()

    create_appimage(
        TARGET fudge
        ICON ${CMAKE_CURRENT_SOURCE_DIR}/assets/fudge.png
        DESKTOP ${CMAKE_CURRENT_SOURCE_DIR}/assets/fudge.desktop
    )
endif()

if(FUDGE_CREATE_MAC_APP)
    function(create_mac_app)
        cmake_parse_arguments(PARSE_ARGV 0 ARG "" "TARGET;APP_NAME;PLIST_PATH;DYLIBS" "")
        if(DYLIBS)
            message(FATAL_ERROR "TODO: use install_name_tool on dylibs")
        endif()
        file(MAKE_DIRECTORY
            "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.app/Contents/MacOS"
            "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.app/Contents/Resources"
            "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.app/Contents/Frameworks"
        )
        add_custom_target(${ARG_APP_NAME}_AppBundle ALL
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${ARG_TARGET}> "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.app/Contents/MacOS/${ARG_APP_NAME}"
            COMMAND chmod +x "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.app/Contents/MacOS/${ARG_APP_NAME}"
            COMMAND ${CMAKE_COMMAND} -E copy ${ARG_PLIST_PATH} "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.app/Contents/Info.plist"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${ARG_TARGET}
        )
        add_custom_target(${ARG_APP_NAME}_Package ALL
            COMMAND tar -czf "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.tar.gz" -C "${CMAKE_CURRENT_BINARY_DIR}" "${ARG_APP_NAME}.app"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${ARG_APP_NAME}_AppBundle
        )
    endfunction()

    create_mac_app(
        TARGET fudge
        APP_NAME fudge
        ICON ${CMAKE_CURRENT_SOURCE_DIR}/assets/fudge.png
        PLIST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/assets/Info.plist
    )
endif()
