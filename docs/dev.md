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

## Smartphone auto transfer (`FUJI_FEATURE_XAPP_WIRELESS_COMM`)
Images can be marked for transfer on the camera (or the camera can mark every new shot). With Bluetooth left on,
the camera keeps advertising while switched off, and xapp downloads the queue in the background. Reproduced on an X100V
from a capture of xapp on iOS, then with a standalone client:

1. Bluetooth: connect with the saved pairing token, then trigger WiFi the same way as `fuji_bluetooth_connect_to_wifi`
   (write `04 00` to the shutter characteristic, read SSID/password). The X100V brings up an open AP.
2. TCP to `192.168.0.1:55740`, init packet, `OpenSession`. The device name in the init packet must be the same name that
   was written to the Bluetooth identity characteristic during pairing, otherwise the camera answers `PTPIP_INIT_FAIL`.
3. Poll `0xd212` every ~200ms. Each reply is a list of events: `u16 count`, then `count` times `u16 code, u32 value`.
   When images are queued the camera reports:
   ```
   0xdf00 = 0x12   (CameraState, FUJI_LIMITED_IMG_TRANSMISSION)
   0xd220 = 1      (SelectedImgsMode)
   0xdf41 = N      (images left in the queue)
   ```
4. Once: set `0xdf01` (ClientState) to `0x15` (`FUJI_MODE_AUTO_TRANSFER_XAPP`), get `0xdf29`, set `0xdf29` to 1.
5. For each image: `GetObjectInfo(1)`, get `0xd235`, then a single `GetPartialObject(1, 0, compressed_size)` for the whole file.
   The handle is always 1. There is no separate acknowledgement: after a complete download the camera moves to the next
   queued image, clears the "transfer" mark on the downloaded one and reports `0xdf41` decremented by one in `0xd212`.
   xapp keeps polling `0xd212` until it sees the new count before requesting the next image.

Notes:
- Downloading the same images in the normal gallery mode (`FUJI_REMOTE_ACCESS`, 1MB `GetPartialObject` chunks) works,
  and the gallery then only lists the marked images, but the camera does not clear the transfer marks.
- One `GetPartialObject` for the whole file is also faster: ~2.8MB/s on the X100V vs ~1.9MB/s with 1MB chunks
  (~12MB JPEGs, same AP, same distance).
- Turning the camera on during the transfer shuts down the AP.
