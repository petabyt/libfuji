# Fudge
This is a cross-platform open-source alternative to Fujifilm's official camera app.

<img src='fastlane/metadata/android/en-US/images/phoneScreenshots/1.png' width='250'><img src='fastlane/metadata/android/en-US/images/phoneScreenshots/2.png' width='250'><img src='fastlane/metadata/android/en-US/images/phoneScreenshots/4.png' width='250'>

This app isn't finished yet, so don't set high expectations. Fuji's implementation of PTP/IP has many different quirks and bugs, so it's not easy to create a stable client that works for all cameras.

## Improvements over XApp / Camera Connect
- Location and notification permissions are *not* required or requested
- More responsive native UI
- Slight performance improvements
- USB-OTG connectivity support
- Supports PC AutoSave and (partially) Wireless Tether

## Missing features
- Bluetooth pairing
- Geolocation
- Liveview/remote shutter

Beta builds are published on [Google Play](https://play.google.com/store/apps/details?id=dev.danielc.fujiapp). The latest builds are also available on [F-Droid](https://apt.izzysoft.de/fdroid/index/apk/dev.danielc.fujiapp).

## Roadmap
- [ ] Desktop app/utility
- [ ] Liveview & Remote capture
- [ ] Camera properties (ISO, white balance, film sim, etc)
- [ ] Implement Bluetooth communication
- [ ] iOS port (see ios/)
- [ ] Support landscape mode/chromeOS/etc

## Compiling
master branch may not compile, clone from the latest release instead.  
### Compiling desktop app
Desktop utility is still a work in progress.
```
cd desktop
cmake -B build -G Ninja
cmake --build build
```
Cross-compiling for Windows with mingw:
```
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/mingw.cmake -G Ninja -B build_win
cmake --build build_win
```

### Compiling Android
Open android/ in Android Studio.
