FUDGE=../lib

CAMLIB_CORE=operations.o packet.o enums.o data.o enum_dump.o lib.o canon.o liveview.o bind.o ip.o ml.o log.o conv.o generic.o canon_adv.o no_usb.o transport.o
CAMLIB=$(addprefix $(FUDGE)/camlib/src/,$(CAMLIB_CORE))

LIB_OBJ=main.o $(FUDGE)/tester.o $(FUDGE)/fuji.o $(FUDGE)/models.o $(CAMLIB)

CFLAGS=-I$(FUDGE) -I$(FUDGE)/camlib/src

main.out: $(LIB_OBJ)
	$(CC) $(LIB_OBJ) $(CFLAGS) -o main.out

clean:
	$(RM) *.o $(LIB_OBJ) main
