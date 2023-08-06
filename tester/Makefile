LIB_OBJ=../lib/models.o

CFLAGS=$(shell pkg-config --libs libusb-1.0) -I../lib/

main: main.o $(LIB_OBJ)
	$(CC) main.o $(LIB_OBJ) -lcamlib $(CFLAGS) -o main

connect:
	systemctl stop NetworkManager
	iwconfig wlp0s20f3 essid "FUJIFILM-X-A2-5DBC"
	dhclient wlp0s20f3

fix:
	systemctl start NetworkManager
