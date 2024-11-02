# About
[I](https://danielc.dev/) started Fudge in [2023](https://github.com/petabyt/fudge/commit/b282b6a8ff5f88c51e1b72333d447b71077545ea) as a response to the release of Fujifilm's Xapp. It got mixed reviews,
both praise and frustrations. This prompted me to try reversing the protocols myself. Once I had finished the PoC, I expanded the project's scope to become a more ambitious replacement for all of Fujifilm's software.

A lot of Fudge discussion happens in the [Fujihack Discord server](https://discord.gg/UZXDktvAZP). We talk about photography, camera mods, and anything related to imaging. Come join if that sounds interesting!

Notes:
- This project was temporarily named 'fujiapp' (hence the package name being dev.danielc.fujiapp)
- 'libfudge' is portable and based on [camlib](https://github.com/petabyt/camlib), a PTP client library I wrote for my experiments with [Magic Lantern](https://github.com/petabyt/mlinstall) and [Fujihack](https://fujihack.org/).

# FAQ
## iOS support?
I ported all of Fudge's PTP code to iOS over a weekend, see [/ios](https://github.com/petabyt/fudge/tree/master/ios). It's missing a frontend, and creating that UI will take a significant
amount of effort.  
There is also [Fuji-Fi](https://apps.apple.com/us/app/fuji-fi/id1441309889) and [Cascable](https://apps.apple.com/us/app/cascable-tether-edit-photos/id974193500) that may work on your camera. Both are closed-source.
## Bluetooth?
- If you are interested in having a seperate piece of hardware for connecting to your camera, take a look at [furble](https://github.com/gkoh/furble).
- A cross platform Bluetooth library should be used, such as [SimpleBLE](https://github.com/OpenBluetoothToolbox/SimpleBLE) or [blue-falcon](https://github.com/Reedyuk/blue-falcon).
- I'd like to implement a virtual BLE camera for debugging/black-box testing, the same as I did for [PTP](https://github.com/petabyt/vcam).
- This feature is currently on the backburner.
## Transfer RAW photos?
- Normal 'Wireless Communication' mode doesn't allow RAW photos to be viewed or downloaded. This is hardcoded in firmware and cannot be changed.
- 'PC AutoSave' allows downloading RAWs over an existing WiFi network.
- If you want the fastest transfer speeds, you can use a USB-C sdcard reader.
## RAW Conversion/Fuji X Raw Studio replacement
This is not high priority right now but it may be exposed over CLI: [#21](https://github.com/petabyt/fudge/issues/21)
