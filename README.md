# fujiapp
This is an alternative reverse-engineered app to Fujifilm's official WiFi app. This is an Android-only app with a very simple goal to serve as a photo viewer
and downloader. It will also serve as a testbed for a portable implementation Fujifilm's superset of PTP/IP, and possibly a more advanced and cross-platform app in the future.

![screenshots](https://eggnog.danielc.dev/f/76-s9xg1g9bj8rm7u1r604z92yy4xyitc.png)

This app isn't finished yet, so don't set high expectations. I'm currently testing it on a 2015 X-A2, but the quirks for newer cameras will slowly be implemented. Bluetooth communication, which is required on the newer models, isn't implemented yet but is on the roadmap.

It will be available [here](https://play.google.com/store/apps/details?id=dev.danielc.fujiapp) once finished.

## Tech stack
- Java & native Android activities
- JNI & Portable C99
- [camlib](https://github.com/petabyt/camlib) with Java communication backend

## Help needed
- UI polishing (animations?)
- WiFi Packet dumps of using either of the official apps (send to brikbusters@gmail.com)
- Writing a complete cross-platform reimplementation of Fujifilm's XApp (with more features) would be a much larger and more complicated effort - email me (brikbusters@gmail.com) if you're interested in making it happen.

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
- [ ] Reimplement UI & in Dart/Flutter, port comm code to iOS and Android

