FUDGE=../lib

MINGW = x86_64-w64-mingw32
CC = $(MINGW)-gcc

OBJ := main.o ip.o ui.o

CAMLIB_CORE := operations.o packet.o enums.o data.o enum_dump.o lib.o canon.o liveview.o bind.o no_ip.o ml.o log.o conv.o generic.o canon_adv.o
CAMLIB_CORE += libwpd.o
OBJ += $(addprefix camlib-,$(CAMLIB_CORE))

FUDGE_CORE := fuji.o tester.o viewer.o
OBJ += $(addprefix fudge-,$(FUDGE_CORE))

CFLAGS := -I$(FUDGE) -I$(FUDGE)/camlib/src

WIN_LIBS += /usr/x86_64-w64-mingw32/lib/libwpd.a
WIN_LIBS += /usr/x86_64-w64-mingw32/lib/libui_win64.dll #/usr/x86_64-w64-mingw32/lib/libui.a
WIN_LIBS += -luser32 -lkernel32 -lgdi32 -lcomctl32 -luxtheme -lmsimg32 -lcomdlg32 -ld2d1 -ldwrite -lole32 -loleaut32 -loleacc
WIN_LIBS += -lstdc++ -lgcc -lpthread -lssp -lurlmon -luuid

fudge.exe: $(OBJ) win.res
	$(CC) $(OBJ) $(CFLAGS) $(WIN_LIBS) win.res -o fudge.exe

win.res: a.rc
	$(MINGW)-windres a.rc -O coff -o win.res

camlib-%.o: $(FUDGE)/camlib/src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

fudge-%.o: $(FUDGE)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	$(RM) *.o $(LIB_OBJ) main *.exe *.res
