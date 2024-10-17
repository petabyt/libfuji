## Overview of Fujifilm's protocols

A lot of this information is available as a writeup here: https://danielc.dev/blog/fudge1/

## Standard PTP/USB (`FUJI_FEATURE_USB_CARD_READER`)
*Also known as USB CARD READER*  
All Fujifilm cams implement at least these PTP opcodes since 2011:
```
0x1001-0x100B, 0x100F, 0x1014, 0x1015, 0x1016, 0x101B
```
Fuji defines 3 custom opcodes:
```
#define PTP_OC_FUJI_SendObjectInfo	0x900c // create file
#define PTP_OC_FUJI_SendObject2		0x900d // Appears to be the same as 901d
#define PTP_OC_FUJI_SendObject		0x901d // write to file
```
These were found in an old FPUPDATE.DAT update utility. These opcodes have the ability to upload AUTO_ACT.SCR and FPUPDATE.DAT files to the sdcard,
something that normal `SendObject` and `SendObjectInfo` cannot do.

## PC AutoSave (`FUJI_FEATURE_AUTOSAVE`)
PC AutoSave is implemented on most cameras since 2014. In 2022, they removed it.
Although it's a dead feature, it's available on most Fuji cameras. Unlike normal WiFi pairing, AutoSave allows
RAF files to be downloaded.

## USB Tether Shoot (`FUJI_FEATURE_USB_TETHER_SHOOT`)
Allows:
- Liveview
- Live downloading full images to client
- Set all common camera settings

## Wireless Tethering (`FUJI_FEATURE_WIRELESS_TETHER`)
Wireless tethering is available from the USB TRANSPORT MODE menu. Selecting it disables USB altogether, and the camera
will immediately try to connect to the WiFi access point that is stored in the camera.
The only major downside is that this mode forces all pictures to be uploaded, and doesn't allow you to choose to save images to card
when the network is not available.
All functionality in this mode is identical to `FUJI_FEATURE_USB_TETHER_SHOOT`, the only difference being the transport.

## USB Raw Conv (`FUJI_FEATURE_RAW_CONV`)
This option allows for:
- Upload RAF images to the camera over USB
- Use the camera's image processor to process images
- Process the final image into a JPEG
- Backup and restore all camera settings
