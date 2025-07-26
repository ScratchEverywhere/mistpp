# Mist++

A port of [TurboWarp/mist](https://github.com/TurboWarp/mist) to C++.

This project was made specifically for
[Scratch 3DS](https://github.com/NateXS/Scratch-3DS).

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
[the mistpp-packages repo](https://github.grady.link/mistpp-packages).
