FUDGE = ../lib

OBJ := main.o ui.o jank.o

CAMLIB_CORE := transport.o operations.o packet.o enums.o data.o enum_dump.o lib.o canon.o liveview.o bind.o ml.o conv.o generic.o canon_adv.o
ifeq ($(TARGET),l)
CAMLIB_CORE += libusb.o
endif
ifeq ($(TARGET),w)
CAMLIB_CORE += libwpd.o
endif
OBJ += $(addprefix camlib-,$(CAMLIB_CORE))
OBJ += $(addprefix camlua-,lua.o runtime.o)
OBJ += $(addprefix camluajson-,lua_cjson.o strbuf.o)

FUDGE_CORE := fuji.o tester.o data.o net.o discovery.o exif.o uilua.o fuji_usb.o
OBJ += $(addprefix fudge-,$(FUDGE_CORE))

CAMLIB_DIR := ../lib/camlib

CFLAGS := -MMD -I$(FUDGE) -I$(CAMLIB_DIR)/src -I$(CAMLIB_DIR)/lua -g

ifeq ($(TARGET),w) # -----------------------
MINGW := x86_64-w64-mingw32
CC := $(MINGW)-gcc

WIN_LIBS += /usr/x86_64-w64-mingw32/lib/libwpd.a /usr/x86_64-w64-mingw32/lib/liblua.a
WIN_LIBS += /usr/x86_64-w64-mingw32/lib/libui_win64.dll
WIN_LIBS += -luser32 -lkernel32 -lgdi32 -lcomctl32 -luxtheme -lmsimg32 -lcomdlg32 -ld2d1 -ldwrite -lole32 -loleaut32 -loleacc -lstdc++ -lgcc -lpthread -lssp -lurlmon -luuid -lws2_32

fudge.exe: $(OBJ) win.res
	$(CC) $(OBJ) $(CFLAGS) $(WIN_LIBS) $(LDFLAGS) win.res -o fudge.exe

win.res: a.rc
	$(MINGW)-windres a.rc -O coff -o win.res
endif # --------------------

ifeq ($(TARGET),l) # ----------------------
CFLAGS += $(shell pkg-config --libs --cflags lua-5.3 libusb-1.0)
LDFLAGS += -lm -lui
fudge.out: $(OBJ)
	$(CC) $(OBJ) $(CFLAGS) $(LDFLAGS) -o fudge.out
endif # ----------------------------

# Manual targets to compile in this dir and not clutter
camlib-%.o: $(CAMLIB_DIR)/src/%.c 
	$(CC) -c $(CFLAGS) $< -o $@

camlua-%.o: $(CAMLIB_DIR)/lua/%.c 
	$(CC) -c $(CFLAGS) $< -o $@

camluajson-%.o: $(CAMLIB_DIR)/lua/lua-cjson/%.c 
	$(CC) -c $(CFLAGS) $< -o $@

fudge-%.o: $(FUDGE)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

-include $(CAMLIB_DIR)/src/*.d $(CAMLIB_DIR)/lua/*.d $(CAMLIB_DIR)/lua/lua-cjson/*.d $(FUDGE)/*.d

clean:
	$(RM) *.o $(LIB_OBJ) main *.exe *.res *.out
