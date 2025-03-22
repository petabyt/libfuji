# Fudge
This is a cross-platform open-source alternative to Fujifilm's official camera app.  

<img src='fastlane/metadata/android/en-US/images/phoneScreenshots/1.png' width='250'><img src='fastlane/metadata/android/en-US/images/phoneScreenshots/2.png' width='250'><img src='fastlane/metadata/android/en-US/images/phoneScreenshots/4.png' width='250'>

This app isn't finished yet, so don't set high expectations. Fuji's implementation of PTP/IP has many different quirks and bugs, so it's not easy to create a stable client that works for all cameras.

## Improvements over XApp / Camera Connect
- Location and notification permissions are *not* required or even requested
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
master branch may not compile or may have unfinished changes. Clone from the last release instead.
```
git clone https://github.com/petabyt/fudge.git --depth 1 --branch 0.2.2 --recurse-submodules
```
### Compiling desktop app
Desktop utility is still a work in progress.
```
# Ubuntu deps:
sudo apt install libusb-1.0-0-dev libimgui-dev libvulkan-dev libglfw3-dev
```
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
