# Fudge
This is a cross-platform open-source alternative to Fujifilm's official camera app.

<img src='fastlane/metadata/android/en-US/images/phoneScreenshots/Screenshot_20240305-104928.png' width='250'><img src='fastlane/metadata/android/en-US/images/phoneScreenshots/Screenshot_20240305-104948.png' width='250'><img src='fastlane/metadata/android/en-US/images/phoneScreenshots/Screenshot_20240305-105032.png' width='250'>

This app isn't finished yet, so don't set high expectations. Fuji's implementation of PTP/IP has many different quirks and bugs, so it's not easy to create a stable client that works for all cameras.

## Improvements over XApp / Camera Connect
- Location and notification permissions are *not* required
- *Much* more responsive native UI
- Performance improvements
- USB-OTG connectivity support
- Lua plugins API

Beta builds are published on [Google Play](https://play.google.com/store/apps/details?id=dev.danielc.fujiapp). The latest builds are also available on [F-Droid](https://apt.izzysoft.de/fdroid/index/apk/dev.danielc.fujiapp).

## Roadmap

- [x] Stable PTP/IP communication with camera over WiFi
- [x] Tested & working on X-A2, X-H1
- [x] Tested & working on a few [virtual](https://github.com/petabyt/vcam) cameras
- [x] Gallery of images
- [x] Zoomable image viewer, download/share photos
- [x] Implement "select multiple / single" mode
- [x] Implement PTP/USB OTG support
- [x] Lua scripting
- [ ] AutoSave and tether mode
- [ ] Mass photo import (different folder for each camera?)
- [ ] Camera properties (ISO, white balance, film sim, etc)
- [ ] Implement Bluetooth communication
- [ ] Liveview & Remote capture
- [ ] Desktop app/utility
- [ ] iOS port (see ios/)
- [ ] Translate UI
- [ ] Support landscape mode

## Libraries
- [camlib](https://github.com/petabyt/camlib)
- Lua 5.3
- libui-android libuilua
- libjpeg-turbo
