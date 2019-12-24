---
title: "Home"
layout: "home"
---

## Building

[![Travis Build Status]](https://travis-ci.com/sfztools/sfizz)

Sfizz depends on the [sndfile] library.
The [JACK] client that you will probably build depends on the `jack` library.
To build `sfizz` you need to install both as shared libraries on the system.
In Debian-based distributions, this translates into

```bash
sudo apt install libjack-jackd2-dev libsndfile1-dev
```

The process is as follows:
1. Clone the repository with all the submodules
2. Create a build directory for CMake and `cd` into it
3. Build
4. Enjoy :)

In the shell world, this means

```bash
git clone --recursive https://github.com/sfztools/sfizz.git
cd sfizz
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

Current configuration switches for CMake are:

```
ENABLE_LTO       Enable Link Time Optimization [default: ON]
SFIZZ_JACK       Enable JACK stand-alone build [default: ON]
SFIZZ_LV2        Enable LV2 plug-in build      [default: ON]
SFIZZ_BENCHMARKS Enable benchmarks build       [default: OFF]
SFIZZ_TESTS      Enable tests build            [default: OFF]
SFIZZ_SHARED     Enable shared library build   [default: ON]
```

For details about building under macOS, see [here].

You can then find the JACK client in `clients/sfizz_jack`.
Just specify an `.sfz` file as a parameter and you are good to go.
The client will forcefully connect to the system output,
and open an event input in JACK for you to connect a midi capable software
or hardware (e.g. `jack-keyboard`).
If no Jack server is already started it will start one with basic options.

## Possible pitfalls and alternatives

If you already cloned the repository without the `--recursive` option,
update the submodules manually with

```bash
git submodule update --init --recursive
```

You can build with `clang`, although in that case the CMakeFile
defaults to using `libc++` instead of `libstdc++`.


[Travis Build Status]: https://img.shields.io/travis/com/sfztools/sfizz.svg?label=Linux-macOS&style=popout&logo=travis
[JACK]:    https://jackaudio.org/
[sndfile]: http://mega-nerd.com/libsndfile/
[here]:    macos
