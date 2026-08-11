# libfuji

C library based on [libpict](https://github.com/petabyt/libpict) to connect to Fujifilm digital cameras over WiFi and USB.

Implements support for:
- Wireless communication mode (Xapp/Camera Connect)
- Legacy and secure Bluetooth pairing modes
- PC Autosave mode
- Wireless tether shoot/USB tether shoot (X Acquire)
- Raw conversion mode (Raw Studio)

Projects using this:
- [fudge](https://github.com/petabyt/fantasyfudge)
- [imgui app](https://github.com/petabyt/fudge-desktop-legacy)

### Legacy Fudge Android App

The legacy version of the Fudge android app is available [here](https://github.com/petabyt/fudge-legacy-android).

In 2026 the Fujifilm-specific C code was split away from the app into this repository, and the existing git history was kept.

## Credits
- [furble](https://github.com/gkoh/furble) - MIT license
