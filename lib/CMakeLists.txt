cmake_minimum_required(VERSION 3.22.1)

project("fujiapp")

include_directories(camlib/src)

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
	add_definitions(-D VCAMERADIR="\\"test/"\\")
	list(APPEND CORE vusb.c vusb/log.c vusb/vcamera.c vusb/libgphoto2_port/gphoto2-port-portability.c)
	include_directories(${PROJECT_SOURCE_DIR}/vusb/)
else()
	list(APPEND CORE net.c)
endif()

add_library(fujiapp
	SHARED
	${CORE}
)

find_library(log-lib log)

target_link_libraries(fujiapp ${log-lib})
