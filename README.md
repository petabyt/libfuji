# fujiapp
This is an alternative reverse-engineered app to communicate with Fujifilm X cameras. It's written in Java,
with the backend being written in C, with [camlib](https://github.com/petabyt/camlib).

This app isn't finished yet, so don't expect anything usable. It will be available [here](https://play.google.com/store/apps/details?id=dev.danielc.fujiapp) once finished.

## Help needed
- UI polishing (animations?)
- WiFi Packet dumps of using the official app (send to me brikbusters@gmail.com)

## TODO:
- [x] Stable communication with camera (connect, disconnect, ping)
- [x] Poll camera for unlocked event
- [x] Tested & working on X-A2
- [x] thumbnail gallery of images
- [x] ZoomView image viewer + button to download to DCIM/fuji
- [x] Downloading progress bar popup
- [x] Share images (Discord doesn't seem to like it)
- [ ] Implement Bluetooth communication (decode encrypted packets)
- [ ] Set GPS location from coordinates
- [ ] Polish UI (more animations)
- [ ] Implement 100s of Fuji property codes (?)
- [ ] Implement liveview (view stream on another port)
- [ ] Redo logo/artwork :)

