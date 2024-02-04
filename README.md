# Fudge
This is a reverse-engineered alternative to Fujifilm's official WiFi app. The short-term goal is to serve as a simple photo gallery and downloader, and focus primarly on reliability.

<img src='fastlane/metadata/android/en-US/images/phoneScreenshots/Screenshot_20230830-213156.png' width='250'><img src='fastlane/metadata/android/en-US/images/phoneScreenshots/Screenshot_20230830-213215.png' width='250'><img src='fastlane/metadata/android/en-US/images/phoneScreenshots/Screenshot_20230830-213428.png' width='250'>

This app isn't finished yet, so don't set high expectations. Fuji's version of PTP/IP has many different quirks and features, so it's not easy to implement them all from
a single camera. Currently, it's only been tested on a few older cameras, but functionality for newer cameras will slowly be implemented (as well as bluetooth) over time.

Beta builds are published on [Google Play](https://play.google.com/store/apps/details?id=dev.danielc.fujiapp). The latest builds are also available on [F-Droid](https://apt.izzysoft.de/fdroid/index/apk/dev.danielc.fujiapp).

## Roadmap
- [x] Stable communication with camera over WiFi
- [x] Tested & working on X-A2
- [x] thumbnail gallery of images
- [x] ZoomView image viewer + button to download to DCIM/fuji
- [x] Working Downloading progress bar
- [x] Download and Share images
- [ ] Delete images?
- [ ] Implement "select multiple / single" mode
- [ ] Translate UI
- [ ] Implement PTP/USB OTG support
- [ ] Lua scripting and automation API
- [ ] Implement Bluetooth communication (decode encrypted packets)
- [ ] iOS port (UIKit, Objective-C)
- [ ] Implement liveview (MJPEG stream on another port)
- [ ] Remote capture
- [ ] Camera properties (ISO, white balance, film sim, etc)

## Tech stack
- Java & native Android activities
- Backend implemented in portable C (JNI)
- [camlib](https://github.com/petabyt/camlib) + Java bindings
- Lua 5.3 + libui & camlib bindings
- LibUI
