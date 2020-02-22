---
title: "Building with MSVC on Windows"
---
The simplest is to use `vcpkg` as a package manager, which will give you access
to `libsndfile`.
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

In the sfizz source directory, you can then build with CMake as usual,
although you should clone the windows branch:
```powershell
git clone --recursive https://github.com/sfztools/sfizz.git
cd sfizz
mkdir build
cd build
cmake .. -DSFIZZ_JACK=OFF "-DCMAKE_TOOLCHAIN_FILE=C:\Users\Paul\source\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static
cmake --build . -j 16 --config Release
```

This builds the lv2 plugin in `build\lv2\Release` and the Turtle files in
`build\sfizz.lv2`, but the installation procedure is not entirely automatic yet.
You need to manually add `lv2\Release\sfizz.dll` to the `build\sfizz.lv2`
directory, and the the `sfizz.lv2` directory to `%APPDATA%\Roaming\LV2`.
Ardour should then find the plugin correctly and load it without issues.

