# Fujifilm Profile parser
Parser, validator, and converter for Fujifilm profile formats

A profile (you could also call it a 'recipe') stores all information needed to produce a jpeg from a raw file.
Most Fuji cameras are capable of applying these profiles to existing raw files, allowing settings for a photo to be altered
afterward and produce jpegs as if they were taken on the scene.

This library aims to be able to parse and validate profile formats used by [X Raw Studio](https://fujifilm-x.com/global/products/software/x-raw-studio/), mainly the
raw data structure sent over PTP/USB.

The results for this project were obtained through black-box reversing with [vcam](https://github.com/petabyt/vcam).

Testing:
```
cmake -DFP_INCLUDE_CLI=ON -G Ninja -B build && cmake --build build && build/test
```

## Profile Formats
### FP1/FP2/FP3 (XML)
XML user profile files created by X Raw Studio
### d185
The data structure sent between X Raw Studio and the camera over PTP
### struct
Intermediate C data structure created for this project for other software to modify

- [x] XML to struct (patched ezxml)
- [x] d185 to struct
- [x] struct to d185
- [ ] struct to XML (split out XML text)
