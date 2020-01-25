# sfizz

[![Travis Build Status](https://img.shields.io/travis/com/sfztools/sfizz.svg?label=Linux&style=popout&logo=travis)](https://travis-ci.com/sfztools/sfizz)

## Building

`sfizz` depends mainly on the `libsndfile` library.
The JACK client depends on the `jack` library.
To build `sfizz` you need to install both as shared libraries on the system.
In Debian-based distributions, this translates into
```
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
- The LV2 plugin with system libraries dynamically loaded

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

## Case issues in SFZ libraries

Most SFZ libraries are built for Windows or macOS and both systems use case-insensitive file systems by default.
This means that SFZ library developers may miss that the sample names used in their file does not have the same case as the ones in their filesystems.
This will be painful for Linux folks.
If you see that this is the case for a library you want to use, you can apply the `sfz_sample_checks.py` script located in the `scripts` directory of this repo to the `.sfz` file.
The usage is as follows:

```sh
usage: sfz_sample_checks.py [-h] [--output OUTPUT] [--test] file

Simple script to update the sample names in an existing SFZ files

positional arguments:
  file             SFZ input file

optional arguments:
  -h, --help       show this help message and exit
  --output OUTPUT  Output file name; if not specified, _corrected will be
                   appended to the input
  --test           Test run
```

Don't hesitate to propose to the sfz library maker to give him the updated file or use the script by himself or herself; this will be transparent for Windows or macOS users and make life of Linux users much easier.

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
