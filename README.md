# libfuji

C library based on [libpict](https://github.com/petabyt/libpict) to connect to Fujifilm digital cameras over WiFi and USB.

This is currently being used for the [Fudge project](https://github.com/petabyt/fudge).

Implements support for:
- Wireless communication mode (Xapp/Camera Connect)
- Legacy and secure Bluetooth pairing modes
- PC Autosave mode
- Wireless tether shoot/USB tether shoot (X Acquire)
- Raw conversion mode (Raw Studio)

### Legacy Fudge Android App

The legacy version of the Fudge android app is available [here](https://github.com/petabyt/fudge-legacy-android).

In 2026 the Fujifilm-specific C code was split away from the app into this repository, and the existing git history was kept.

## Credits
- [furble](https://github.com/gkoh/furble) - MIT license
