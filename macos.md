---
title: "macOS Build"
---

Currently we don't use / own a macOS machine, so all we can do is to build our
code with Travis, so any Apple user's contribution is welcome.

## Configuration

Update [Homebrew], install [CMake] and [libsndfile] if missing.

```bash
brew update
brew upgrade cmake
brew install libsndfile
```

[JACK] is also required if you are going to build the client.

```bash
brew install jack
```

## make

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
```

## XCode

Usually the XCode build is called with something like:

```bash
mkdir build && cd build
cmake -G Xcode ..
xcodebuild -project sfizz.xcodeproj -alltargets -configuration Release build
```

But unfortunately there is an [issue] currently building with XCode 10+
using CMake, so we are using `make` instead.


[CMake]:      https://cmake.org/
[Homebrew]:   https://brew.sh/
[issue]:      https://gitlab.kitware.com/cmake/cmake/issues/18088
[JACK]:       https://jackaudio.org/
[libsndfile]: http://mega-nerd.com/libsndfile/
