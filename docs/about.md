# FAQ
## iOS support?
I ported all of Fudge's PTP code to iOS over a weekend, see [/ios](https://github.com/petabyt/fudge/tree/master/ios). It's missing a frontend, and creating that UI will take a significant
amount of effort.  
There is also [Fuji-Fi](https://apps.apple.com/us/app/fuji-fi/id1441309889) and [Cascable](https://apps.apple.com/us/app/cascable-tether-edit-photos/id974193500). Both are closed-source.
## Bluetooth?
- If you are interested in having a seperate piece of hardware for connecting to your camera, take a look at [furble](https://github.com/gkoh/furble).
- I'd like to implement a virtual BLE camera for debugging/black-box testing, the same as I did for [PTP](https://github.com/petabyt/vcam).
- A cross platform Bluetooth library should be used, such as [SimpleBLE](https://github.com/OpenBluetoothToolbox/SimpleBLE) or [blue-falcon](https://github.com/Reedyuk/blue-falcon).
## Transfer RAW photos?
- Normal 'Wireless Communication' mode doesn't allow RAW photos to be viewed or downloaded. This is hardcoded and cannot be changed.
- 'PC AutoSave' allows downloading RAWs over an existing WiFi network.
- If you want the fastest transfer speeds, you can use a USB-C sdcard reader.
## RAW Conversion/Fuji X Raw Studio replacement
This may be baked into a desktop client, possibly through CLI options.
See [#21](https://github.com/petabyt/fudge/issues/21)

# About
- The Fudge project was started in [June of 2023](https://github.com/petabyt/fudge/commit/b282b6a8ff5f88c51e1b72333d447b71077545ea)
- This project was temporarily named 'fujiapp' (hence the package name being dev.danielc.fujiapp)
- 'libfudge' is portable and based on [camlib](https://github.com/petabyt/camlib), a PTP client library I wrote.

A lot of Fudge discussion happens in the [Fujihack Discord server](https://discord.gg/UZXDktvAZP). We talk about photography, camera mods, and anything related to imaging. Come join if that sounds interesting!
