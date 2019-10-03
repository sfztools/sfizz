---
title: "Home"
lang: "en"
layout: "home"
date_fmt: "%B %d, %Y"
---

## Building

[![Travis Build Status](https://img.shields.io/travis/com/sfztools/sfizz.svg?label=Linux&style=popout&logo=travis)](https://travis-ci.com/sfztools/sfizz)

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
3. Prep the Makefiles with the `SFIZZ_CLIENTS` option
4. Build
5. Enjoy :)

In the shell world, this means
```sh
git clone --recursive https://github.com/sfztools/sfizz.git
cd sfizz
mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=Release -D SFIZZ_CLIENTS=ON ..
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

<!--
## Latest News

{% include post.html %}
-->
