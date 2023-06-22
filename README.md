# fujiapp
This is an alternative reverse-engineered app to communicate with Fujifilm X cameras. It's written in Java,
with the backend being written in C, with [camlib](https://github.com/petabyt/camlib).

This app isn't finished yet, so don't expect anything usable. It does function though, and will be available [here](https://play.google.com/store/apps/details?id=dev.danielc.fujiapp) once finished.

## TODO:
- [x] Stable communication with camera (connect, disconnect, ping)
- [x] Poll camera for unlocked event
- [x] Tested on X-A2
- [x] thumbnail gallery of images
- [x] ZoomView image viewer + button to download to DCIM/fuji
- [ ] Downloading progress bar popup
- [ ] Share images (buggy)
- [ ] Implement Bluetooth communication (decode encrypted packets)
- [ ] Set GPS location from coordinates
- [ ] Try to update firmware with 0x900c/0x901d
- [ ] Polish UI (more animations, remove logs everywhere)
- [ ] Implement 100s of Fuji property codes (?)
- [ ] Implement liveview (view stream on another port)
- [ ] Redo logo/artwork :)
