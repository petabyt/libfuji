include cross.mk

FUDGE = ../lib

CAMLIB_CORE := transport.o operations.o packet.o enums.o data.o enum_dump.o lib.o canon.o liveview.o bind.o ml.o conv.o generic.o canon_adv.o
OBJ := main.o ui.o jank.o wifi.o backend.o
FUDGE_CORE := fuji.o tester.o data.o net.o discovery.o exif.o uilua.o fuji_usb.o object.o

ifeq ($(TARGET),l)
CAMLIB_CORE += libusb.o
OBJ += unix.o ci.o
endif
ifeq ($(TARGET),w)
CAMLIB_CORE += libwpd.o
OBJ += win.o
endif

OBJ += $(addprefix camlib-,$(CAMLIB_CORE))
OBJ += $(addprefix camlua-,lua.o runtime.o)
OBJ += $(addprefix camluajson-,lua_cjson.o strbuf.o)
OBJ += $(addprefix fudge-,$(FUDGE_CORE))
OBJ := $(call convert_target,$(OBJ))

CAMLIB_DIR := ../lib/camlib

CFLAGS := -I$(FUDGE) -I$(CAMLIB_DIR)/src -I$(CAMLIB_DIR)/src/lua -g

# -fsanitize=address -static-libasan

ifeq ($(TARGET),w) # -----------------------
WIN_LIBS += $(LIBWPD_A) $(LIBLUA_A) $(LIBUI_DLL)
WIN_LIBS += $(WIN_LINK_ESSENTIALS) -lstdc++ -lgcc -lpthread

fudge.exe: $(OBJ) win.res
	$(CC) $(OBJ) $(CFLAGS) $(WIN_LIBS) $(LDFLAGS) win.res -o fudge.exe
endif # --------------------

ifeq ($(TARGET),l) # ----------------------
CFLAGS += $(shell pkg-config --libs --cflags lua-5.3 libusb-1.0)
LDFLAGS += -lm -lui
fudge.out: $(OBJ)
	$(CC) $(OBJ) $(CFLAGS) $(LDFLAGS) -o fudge.out
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

-include $(CAMLIB_DIR)/src/*.d $(CAMLIB_DIR)/lua/*.d $(CAMLIB_DIR)/lua/lua-cjson/*.d $(FUDGE)/*.d

clean:
	$(RM) *.o $(LIB_OBJ) main *.exe *.res *.out *.d
