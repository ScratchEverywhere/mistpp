# Mist++

A port of [TurboWarp/mist](https://github.com/TurboWarp/mist) to C++.

This project was made specifically for
[Scratch 3DS](https://github.com/ScratchEverywhere/ScratchEverywhere).

## Installation

### PC

```sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE="Release" -DBUILD_TEST=off
make
sudo make install
```

### DevkitPro

Use the packages in
[the mistpp-packages repo](https://github.com/ScratchEverywhere/mistpp-packages).
