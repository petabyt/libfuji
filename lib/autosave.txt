## Register (PC AUTO SAVE SETTING)
1. Camera sends DISCOVER to devices listening to datagrams on port 51542
```
DISCOVER * HTTP/1.1
MX:3
HOST:192.168.1.255:|
DSCADDR:192.168.1.32
SERVICE:PCAUTOSAVE/1.0
```
2. Clients must connect to the device on port 51540 and send NOTIFY - camera responds with HTTP OK
```
NOTIFY * HTTP/1.1
HOST: 192.168.1.32:51540
IMPORTER: desktop
```
`HTTP/1.1 200 OK`
3. Client opens TCP server on 51542, starts listening to camera requests:
  - REGISTER * HTTP -> Client responds with HTTP OK, guest, servicename
```
REGISTER * HTTP/1.1
HOST:192.168.1.2:51542
DSCNAME:FUJIFILM-X-H1-CA2A
DSCMODEL:X-H1
```
```
HTTP/1.1 200 OK
FOLDER: guest
ServiceName: PCAUTOSAVE/1.0
```
All connections closed.

## Connect (PC AUTO SAVE)
1. Camera sends DISCOVER to devices listening to datagrams on port 51541
```
DISCOVER desktop HTTP/1.1
MX:3
HOST:192.168.1.255:|
DSCADDR:192.168.1.32
SERVICE:PCAUTOSAVE/1.0
```
2. Clients must connect to the device on port 51540 and send NOTIFY - camera responds with HTTP OK
```
NOTIFY * HTTP/1.1
HOST: 192.168.1.32:51540
IMPORTER: desktop
```
`HTTP/1.1 200 OK`
3. Client opens TCP server on 51541, starts listening to camera requests:
 - IMPORT /guest, DSPORT:$ -> Client responds with HTTP OK
```
IMPORT /guest HTTP/1.1
HOST:192.168.1.2:51541
DSCNAME:FUJIFILM-X-H1-CA2A
DSCPORT:$
```
`HTTP/1.1 200 OK`
4. After sending this HTTP OK, the client connects to FUJI_CMD_IP_PORT.


PropCode D228

06 2000 3000 2F00 3600 3000 0000

0E 2000 3000 2F00 3600 3000 3A00 4500 5200 5200 2800 2D00 3100 2900 0000
