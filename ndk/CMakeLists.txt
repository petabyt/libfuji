cmake_minimum_required(VERSION 3.22.1)

project(fudge)

# libfudge
option(PTP_USE_LIBUSB "" OFF)
option(PTP_DEFAULT_LOGGING "" OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib ${CMAKE_CURRENT_BINARY_DIR}/lib)

# libjpeg-turbo
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/android-libjpeg-turbo ${CMAKE_CURRENT_BINARY_DIR}/libjpeg)

add_library(fudge SHARED
	main.c
	backend.c
	scripts.c
	camlib.c
	usb.c
	liveview.c
	android.c
)

# Define BUILD_TYPE_RELEASE for release builds
if(${CMAKE_BUILD_TYPE} STREQUAL "Release" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
	target_compile_definitions(fudge PUBLIC BUILD_TYPE_RELEASE=1)
endif()

target_compile_options(fudge
    PRIVATE -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations
)

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(glesv2-lib GLESv2)
find_library(glesv1-lib GLESv1_CM)
target_link_libraries(fudge PRIVATE libfudge
	libjpeg-turbo
	${log-lib}
	${android-lib}
	${egl-lib}
	${glesv2-lib}
	${glesv1-lib}
)
