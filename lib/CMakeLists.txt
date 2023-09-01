cmake_minimum_required(VERSION 3.22.1)

project("fujiapp")

include_directories(camlib/src)
include_directories(.)

add_definitions(-D FUJI_VUSB)

set(CORE
	camlib/src/backend.c
	camlib/src/bind.c
	camlib/src/data.c
	camlib/src/enum_dump.c
	camlib/src/enums.c
	camlib/src/fuji.c
	camlib/src/canon.c
	camlib/src/liveview.c
	camlib/src/no_usb.c
	camlib/src/operations.c
	camlib/src/packet.c
	camlib/src/util.c
	camlib/src/ml.c
	camlib/src/conv.c

	main.c
	jni.c
	lib.c
	fuji.c
	tester.c
	models.c
)

#set(USE_VUSB 1)

if(DEFINED USE_VUSB)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
	    message(FATAL_ERROR "Disable vcam!")
	endif()

	add_definitions(-D VCAMERADIR="\\"test/"\\")
	list(APPEND CORE vusb.c vusb/src/log.c vusb/src/vcamera.c vusb/src/libgphoto2_port/gphoto2-port-portability.c vusb/src/fuji.c)
	include_directories(${PROJECT_SOURCE_DIR}/vusb/src/)
else()
	list(APPEND CORE net.c)
endif()

add_library(fujiapp
	SHARED
	${CORE}
)

find_library(log-lib log)

target_link_libraries(fujiapp ${log-lib})
