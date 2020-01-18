# sfizz

[![Travis Build Status](https://img.shields.io/travis/com/sfztools/sfizz.svg?label=Linux&style=popout&logo=travis)](https://travis-ci.com/sfztools/sfizz)

## Building

`sfizz` depends on the `libsndfile` library.
The JACK client depends on the `jack` library.
To build `sfizz` you need to install both as shared libraries on the system.
In Debian-based distributions, this translates into
```
sudo apt install libjack-jackd2-dev libsndfile1-dev
```

The process is as follows:
1. Clone the repository with all the submodules
2. Create a build directory for CMake and `cd` into it
3. Build as release
4. Enjoy :)

In the shell world, this means
```sh
git clone --recursive https://github.com/sfztools/sfizz.git
git checkout develop
cd sfizz
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

By default this builds and installs:
- The shared library version of sfizz
- The JACK client
- The LV2 plugin

The JACK client client will forcefully connect to the system output, and open an event input in Jack for you to connect a midi capable software or hardware (e.g. `jack-keyboard`).

Note that you can disable all targets but the LV2 plugin using
```sh
cmake -DCMAKE_BUILD_TYPE=Release -DSFIZZ_JACK=OFF -DSFIZZ_SHARED=OFF ..
```
and process as before.

### Possible pitfalls and alternatives

If you already cloned the repository without the `--recursive` option, update the submodules manually with
```
git submodule update --init --recursive
```

You can build with `clang`, although in that case the CMakeFile defaults to using `libc++` instead of `libstdc++`.

### Building with MSVC on windows

The simplest is to use `vcpkg` as a package manager, which will give you access to `libsndfile`.
```powershell
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg integrate powershell
```

Assuming you want to build for x64, install the relevant packages as follows
```powershell
.\vcpkg.exe install libsndfile:x64-windows-static zlib:x64-windows-static libsamplerate:x64-windows-static
```

In the sfizz source directory, you can then build with CMake as usual, although you should clone the windows branch:
```powershell
git clone --recursive https://github.com/sfztools/sfizz.git
cd sfizz
mkdir build
cd build
cmake .. -DSFIZZ_JACK=OFF "-DCMAKE_TOOLCHAIN_FILE=C:\Users\Paul\source\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static
cmake --build . -j 16 --config Release
```

This builds the lv2 plugin in `build\lv2\Release` and the Turtle files in `build\sfizz.lv2`, but the installation procedure is not entirely automatic yet.
In particular, having the required DLLs along with `sfizz.dll` is actually not helpful because the system cannot find dlls from another dll's root directory.
You thus need to add all of `FLAC.dll`, `libsndfile-1.dll`, `vorbis.dll`, `ogg.dll`, `vorbisenc.dll` in some directory in your `PATH` variable, and manually add `sfizz.dll` to the `sfizz.lv2` directory, and the the `sfizz.lv2` directory to `%APPDATA%\Roaming\LV2`.
Ardour should then find the plugin correctly and load it without issues.
