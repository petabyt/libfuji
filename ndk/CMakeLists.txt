cmake_minimum_required(VERSION 3.22.1)

project(fudge)

message(STATUS ${ANDROID})
if(NOT ANDROID)
	message(FATAL_ERROR "Platform is not android (${ANDROID}) (${CMAKE_ANDROID_ARCH_ABI})")
endif()

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
target_compile_options(fudge
    PRIVATE -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations
)

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(glesv2-lib GLESv2)
find_library(glesv1-lib GLESv1_CM)
target_link_libraries(fudge PRIVATE libfudge libjpeg-turbo ${log-lib}
		${android-lib}
		${egl-lib}
		${glesv2-lib}
		${glesv1-lib})
