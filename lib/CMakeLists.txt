cmake_minimum_required(VERSION 3.22.1)

project("fujiapp")

include_directories(camlib/src)

add_library(fujiapp
SHARED
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

find_library(log-lib log)

target_link_libraries(fujiapp ${log-lib})
