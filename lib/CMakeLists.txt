cmake_minimum_required(VERSION 2.4.4...3.5.0)

project(libfudge)
option(LIBFUDGE_IMPORT_LIBXML "" ON)
option(LIBFUDGE_IMPORT_LUA "" ON)

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

if(LIBFUDGE_IMPORT_LUA)
    include(../third_party/lua.cmake)
endif()

if(LIBFUDGE_IMPORT_LIBXML)
    option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
    option(LIBXML2_WITH_CATALOG "Add the Catalog support" OFF)
    option(LIBXML2_WITH_DEBUG "Add the debugging module" OFF)
    option(LIBXML2_WITH_HTML "Add the HTML support" OFF)
    option(LIBXML2_WITH_HTTP "Add the HTTP support" OFF)
    option(LIBXML2_WITH_ICONV "Add ICONV support" OFF)
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
    set(LIBXML2_LIBRARIES LibXml2::LibXml2)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/camlib ${CMAKE_CURRENT_BINARY_DIR}/camlib)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fp ${CMAKE_CURRENT_BINARY_DIR}/fp)

target_include_directories(libfudge PUBLIC ${PROJECT_SOURCE_DIR} ${LUA_INCLUDE_DIRS})
target_link_libraries(libfudge PUBLIC libcamlib fp lua ${LUA_LIBRARIES})
target_compile_definitions(libfudge PUBLIC ${LUA_CFLAGS_OTHER})
set_target_properties(libfudge PROPERTIES OUTPUT_NAME libfudge)
