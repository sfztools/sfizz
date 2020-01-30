# sfizz

[![Travis Build Status](https://img.shields.io/travis/com/sfztools/sfizz.svg?label=Linux&style=popout&logo=travis)](https://travis-ci.com/sfztools/sfizz)

## Building

Most people will probably want the LV2 plugin with `libsndfile` built-in statically.
You can directly build it this way through Docker by calling these in an *empty* directory :
```
wget https://raw.githubusercontent.com/sfztools/sfizz/master/scripts/Dockerfile
wget https://raw.githubusercontent.com/sfztools/sfizz/master/scripts/x64-linux-hidden.cmake
docker build -t sfizz .
docker cp $(docker create sfizz:latest):/tmp/sfizz/build/sfizz.lv2 .
```
Note that the statically linked LV2 plugin is to be distributed under the LGPL license, as per the terms of the `libsndfile` library.

### More generic builds and development

`sfizz` depends mainly on the `libsndfile` library.
To build the other `sfizz` targets you need to install both `libsndfile` and `JACK` as shared libraries on the system.
In Debian-based distributions, this translates into

```bash
sudo apt install libjack-jackd2-dev libsndfile1-dev
```
The benchmarks depend on the `benchmark` library (https://github.com/google/benchmark).
If you wish to build the benchmarks you should either build it from source and install the static library, or use the library from your distribution---Ubuntu proposes a `libbenchmark-dev` package that does this.

The process is as follows:
1. Clone the repository with all the submodules
2. Create a build directory for CMake and `cd` into it
3. Build as release
4. Enjoy :)

In the shell world, this means

```bash
git clone --recursive https://github.com/sfztools/sfizz.git
git checkout develop
cd sfizz
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

By default this builds and installs:
- The shared library version of sfizz with both C and C++ interfaces
- The JACK client

The JACK client client will forcefully connect to the system output, and open an event input in Jack for you to connect a midi capable software or hardware (e.g. `jack-keyboard`).

Note that you can disable all targets but the LV2 plugin using
```sh
cmake -DCMAKE_BUILD_TYPE=Release -DSFIZZ_JACK=OFF -DSFIZZ_SHARED=OFF ..
```
and process as before.
In this case, the LV2 plugin will load `libsndfile` dynamically from your system.

## Possible pitfalls and alternatives

If you already cloned the repository without the `--recursive` option,
update the submodules manually with

```bash
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
.\vcpkg.exe install libsndfile:x64-windows-static benchmark:x64-windows-static
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

### Building the LV2 plugin with static linkage to `libsndfile` on Linux

This uses Docker and `vcpkg` on Linux.
Install a Docker version and use the `Dockerfile` located in `scripts/`.
The `scripts/` directory also contains a `docker_lv2_release.sh` file that automates downloading the current `develop` branch and building an LV2 release plugin.
Note that the statically linked LV2 plugin is to be distributed under the LGPL license, as per the terms of the `libsndfile` library.

## License and contribution information

Contributors to `sfizz` include:
- Paul Ferrand (2019-) (maintainer)
- Andrea Zanellato (2019-) (devops, documentation and distribution)
- Jean-Pierre Cimalando (2020-)
- Michael Willis (2020-)
- Alexander Mitchell (2020-)

The sfizz library makes primary use of:
- [libsndfile](https://github.com/erikd/libsndfile/) (licensed under the GNU Lesser General Public License v2.1)
- [Abseil](https://github.com/abseil/abseil-cpp) (licensed under the Apache License 2.0)
- [atomic_queue](https://github.com/max0x7ba/atomic_queue) by Maxim Egorushkin (licensed under the MIT license)
- [filesystem](https://github.com/gulrak/filesystem) by Steffen Schümann (licensed under the BSD 3-Clause license)
- [hiir](http://ldesoras.free.fr/prod.html#src_hiir) by Laurent de Soras (licensed under the Do What The Fuck You Want To Public License v2 license)

The sfizz library also uses in some subprojects:
- [Catch2](https://github.com/catchorg/Catch2)  (licensed under the Boost Software License 1.0)
- [benchmark](https://github.com/google/benchmark) (licensed under the Apache License 2.0)
- [LV2](https://lv2plug.in/) (licensed under the ISC license)
- [JACK](https://github.com/jackaudio/jack2) (licensed under the GNU Lesser General Public License v2.1)
- `neon_mathfun.h` and `sse_mathfun.h` by Julien Pommier (licensed under the zlib license)
