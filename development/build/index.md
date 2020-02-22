---
title: "Building"
---

[![Travis Build Status]](https://travis-ci.com/sfztools/sfizz)

We use [CMake] as build system.
Current configuration switches for CMake are:

```
ENABLE_LTO       Enable Link Time Optimization [default: ON]
SFIZZ_JACK       Enable JACK stand-alone build [default: ON]
SFIZZ_LV2        Enable LV2 plug-in build      [default: ON]
SFIZZ_BENCHMARKS Enable benchmarks build       [default: OFF]
SFIZZ_TESTS      Enable tests build            [default: OFF]
SFIZZ_SHARED     Enable shared library build   [default: ON]
```

The process is as follows:
1. Clone the repository with all the submodules
2. Create a build directory for CMake and `cd` into it
3. Build as release
4. Enjoy :)

In the shell world, this means

```bash
git clone --recursive https://github.com/sfztools/sfizz.git
cd sfizz
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

If you already cloned the repository without the `--recursive` option,
update the submodules manually with

```bash
git submodule update --init --recursive
```

For details about building under macOS, see [here].
There is also a guide for [Windows].

By default this builds and installs:
- The shared library version of sfizz with both C and C++ interfaces
- The JACK client

You can then find the JACK client in `clients/sfizz_jack`.
Just specify an `.sfz` file as a parameter and you are good to go.
The JACK client client will forcefully connect to the system output,
and open an event input in Jack for you to connect a midi capable software
or hardware (e.g. `jack-keyboard`).
If no Jack server is already started it will start one with basic options.

Note that you can disable all targets but the LV2 plugin using

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DSFIZZ_JACK=OFF -DSFIZZ_SHARED=OFF ..
```

and process as before.
In this case, the LV2 plugin will load `libsndfile` dynamically from your system.

You can build with `clang`, although in that case the CMakeFile
defaults to using `libc++` instead of `libstdc++`.

## Building the LV2 plugin with static linkage to `libsndfile` on Linux

Most people will probably want the LV2 plugin with `libsndfile` built-in statically.
You can directly build it this way through Docker by calling these in an *empty* directory :
```
wget https://raw.githubusercontent.com/sfztools/sfizz/master/scripts/Dockerfile
wget https://raw.githubusercontent.com/sfztools/sfizz/master/scripts/x64-linux-hidden.cmake
docker build -t sfizz .
docker cp $(docker create sfizz:latest):/tmp/sfizz/build/sfizz.lv2 .
```
Note that the statically linked LV2 plugin is to be distributed under
the LGPL license, as per the terms of the `libsndfile` library.

This uses Docker and `vcpkg` on Linux.
Install a Docker version and use the `Dockerfile` located in `scripts/`.
The `scripts/` directory also contains a `docker_lv2_release.sh` file that
automates downloading the current `develop` branch and building an LV2 release plugin.

[CMake]:      https://cmake.org/
[here]:       macos
[Windows]:    windows
[Travis Build Status]: https://img.shields.io/travis/com/sfztools/sfizz.svg?label=Linux-macOS&style=popout&logo=travis
