# libfuji

C library based on [libpict](https://github.com/petabyt/libpict) to connect to Fujifilm digital cameras over WiFi and USB.

This implements Fujifilm's proprietary TCP protocol based on PTP-USB, and has functionality that their software (Xapp, Camera Connect, TETHER APP, X Acquire, Raw Studio, PC AutoSave, X Webcam) implements.

Capabilities:
- Implements partial or full support for:
  - Wireless Communication mode (Xapp, Camera Remote app)
    - Supports remote mode and legacy modes
  - PC AutoSave (discovery & downloading)
  - Wireless tether shoot & USB tether shoot
  - Raw Conversion mode
    - also see [fp](https://github.com/petabyt/fp) for FP/XML parsing
- Fully thread safe

For example using in a desktop application, see [fudge-desktop-legacy](https://github.com/petabyt/fudge-desktop-legacy)

### Legacy Fudge Android App

The legacy version of the Fudge android app is available in the [android-app-final](https://github.com/petabyt/libfuji/tree/android-app-final) branch.

In 2026 the Fujifilm-specific C code was split away from the app into this repository, and the existing git history was kept.

All release apks are available for download here: https://s1.danielc.dev/fudge-legacy-apks/
