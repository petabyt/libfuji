all: main_target
include cross.mk

all: fudge.out

cmake_win:
	cmake -DCMAKE_TOOLCHAIN_FILE=mingw.cmake -G Ninja -B build_win
	cmake --build build_win

FUDGE = ../lib

CAMLIB_CORE := transport.o operations.o packet.o enums.o data.o enum_dump.o lib.o canon.o liveview.o bind.o ml.o conv.o generic.o canon_adv.o stuff.o
OBJ := main.o jank.o backend.o
FUDGE_CORE := fuji.o tester.o data.o net.o discovery.o exif.o fuji_usb.o object.o fuji_lua.o lua_runtime.o

ifeq ($(TARGET),l)
CAMLIB_CORE += libusb.o
OBJ += unix.o ci.o
endif
ifeq ($(TARGET),w)
CAMLIB_CORE += libwpd.o
OBJ += win.o
endif

OBJ += $(addprefix camlib-,$(CAMLIB_CORE))
OBJ += $(addprefix camlua-,lua.o)
OBJ += $(addprefix fp-,xml.o data.o d185.o)
OBJ += $(addprefix camluajson-,lua_cjson.o strbuf.o)
OBJ += $(addprefix fudge-,$(FUDGE_CORE))
OBJ := $(call convert_target,$(OBJ))

CAMLIB_DIR := ../lib/camlib

CFLAGS := -I$(FUDGE) -I$(CAMLIB_DIR)/src -I$(CAMLIB_DIR)/src/lua -I$(FUDGE)/fp/src -g

# -fsanitize=address -static-libasan

ifeq ($(TARGET),w) # -----------------------
WIN_LIBS += $(LIBWPD_A) $(LIBLUA_A) $(LIBUI_A)
WIN_LIBS += $(WIN_LINK_ESSENTIALS) -lstdc++ -lgcc -lpthread

fudge.exe: $(OBJ) win.res
	$(CPP) -s -static $(OBJ) $(CFLAGS) $(WIN_LIBS) $(LDFLAGS) win.res -o fudge.exe
endif # --------------------

ifeq ($(TARGET),l) # ----------------------
CFLAGS += $(shell pkg-config --cflags lua-5.3 libusb-1.0 libxml-2.0)
LDFLAGS += -lm -lui $(shell pkg-config --libs lua-5.3 libxml-2.0)
LDFLAGS += $(shell pkg-config --libs libusb-1.0)
#LDFLAGS += -lusb-vcam
fudge.out: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o fudge.out

main_target: fudge.out

endif # ----------------------------

# Manual targets to compile in this dir
camlib-%.$(TARGET).o: $(CAMLIB_DIR)/src/%.c
	$(CC) -c -MMD $(CFLAGS) $< -o $@

camlua-%.$(TARGET).o: $(CAMLIB_DIR)/src/lua/%.c 
	$(CC) -c -MMD $(CFLAGS) $< -o $@

camluajson-%.$(TARGET).o: $(CAMLIB_DIR)/src/lua/lua-cjson/%.c 
	$(CC) -c -MMD $(CFLAGS) $< -o $@

fudge-%.$(TARGET).o: $(FUDGE)/%.c
	$(CC) -c -MMD $(CFLAGS) $< -o $@

fp-%.$(TARGET).o: $(FUDGE)/fp/src/%.c
	$(CC) -c -MMD $(CFLAGS) $< -o $@

-include $(CAMLIB_DIR)/src/*.d $(CAMLIB_DIR)/lua/*.d $(CAMLIB_DIR)/lua/lua-cjson/*.d $(FUDGE)/*.d

clean:
	$(RM) *.o $(LIB_OBJ) main *.exe *.res *.out *.d
