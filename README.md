# sfizz

[![Travis Build Status](https://img.shields.io/travis/com/sfztools/sfizz.svg?label=Linux&style=popout&logo=travis)](https://travis-ci.com/sfztools/sfizz)

## Building

Sfizz depends on the `sndfile` library.
The Jack client that you will probably build depends on the `jack` library.
To build `sfizz` you need to install both as shared libraries on the system.
In Debian-based distributions, this translates into
```
sudo apt install libjack-jackd2-dev libsndfile1-dev
```

The process is as follows:
1. Clone the repository with all the submodules
2. Create a build directory for CMake and `cd` into it
3. Build
4. Enjoy :)

In the shell world, this means
```sh
git clone --recursive https://github.com/sfztools/sfizz.git
cd sfizz
mkdir build && cd build
cmake ..
make
```
You can then find the Jack client in `clients/sfizz_jack`.
Just specify an `.sfz` file as a parameter and you are good to go.
The client will forcefully connect to the system output, and open an event input in Jack for you to connect a midi capable software or hardware (e.g. `jack-keyboard`).
If no Jack server is already started it will start one with basic options.

### Possible pitfalls and alternatives

If you already cloned the repository without the `--recursive` option, update the submodules manually with
```
git submodule update --init --recursive
```

You can build with `clang`, although in that case the CMakeFile defaults to using `libc++` instead of `libstdc++`.

### Building with MSVC on windows

The simplest is to use `vcpkg` as a package manager, which will give you access to `libsndfile`.
```sh
PS> git clone https://github.com/Microsoft/vcpkg.git
PS> cd vcpkg
PS> .\bootstrap-vcpkg.bat
PS> .\vcpkg integrate install
PS> .\vcpkg integrate powershell
```

Assuming you want to build for x64, install the relevant packages as follows
```sh
PS> .\vcpkg.exe install libsndfile:x64-windows zlib:x64-windows libsamplerate:x64-windows
```

In the sfizz source directory, you can then build with CMake as usual, although you should clone the windows branch:
```sh
git clone --branch windows --recursive https://github.com/sfztools/sfizz.git
cd sfizz
mkdir build && cd build
cmake .. -DSFIZZ_JACK=OFF "-DCMAKE_TOOLCHAIN_FILE=C:\Users\Paul\source\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build . -j 16 --config Release
```

