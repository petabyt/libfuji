# libfuji

C library based on [libpict](https://github.com/petabyt/libpict) to connect to Fujifilm digital cameras over WiFi and USB. This implements
Fujifilm's proprietary TCP protocol based on PTP-USB, and has functionality that their software (Xapp, Camera Connect, TETHER APP, X Acquire, Raw Studio, PC AutoSave, X Webcam) implements.

Capabilities:
- WiFi: Implements 'Remote mode' as well as some legacy modes
- WiFi: View mjpeg liveview and change settings
- Implements legacy PC AutoSave functionality
- Implements TETHER APP photo downloading over WiFi functionality 
- Implements USB raw conversion protocol (also see [fp](https://github.com/petabyt/fp) for FP/XML parsing)
- Query photos on the card and get thumbnail and metadata
- Download photos and videos from card

### Legacy Fudge Android App

The legacy version of the Fudge android app is available in the [android-app-final](https://github.com/petabyt/libfuji/tree/android-app-final) branch.

In 2026 the Fujifilm-specific C code was split away from the app into this repository, and the existing git history was kept.

All release apks are available for download here: https://s1.danielc.dev/fudge-legacy-apks/
