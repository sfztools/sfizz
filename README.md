# sfizz

[![Travis Build Status]](https://travis-ci.com/sfztools/sfizz)
[![AppVeyor Build Status]](https://ci.appveyor.com/project/sfztools/sfizz)

SFZ library and LV2 plugin, please check [our website] for more details.

![Screenshot](screenshot.png)

## Dependencies and licenses

The sfizz library makes primary use of:
- [libsndfile], licensed under the GNU Lesser General Public License v2.1
- [Abseil], licensed under the Apache License 2.0
- [atomic_queue] by Maxim Egorushkin, licensed under the MIT license
- [filesystem] by Steffen Schümann, licensed under the BSD 3-Clause license
- [hiir] by Laurent de Soras, licensed under the WTFPL v2 license

The sfizz library also uses in some subprojects:
- [Catch2], licensed under the Boost Software License 1.0
- [benchmark], licensed under the Apache License 2.0
- [LV2], licensed under the ISC license
- [JACK], licensed under the GNU Lesser General Public License v2.1

[Abseil]:       https://github.com/abseil/abseil-cpp
[atomic_queue]: https://github.com/max0x7ba/atomic_queue
[benchmark]:    https://github.com/google/benchmark
[Catch2]:       https://github.com/catchorg/Catch2
[filesystem]:   https://github.com/gulrak/filesystem
[hiir]:         http://ldesoras.free.fr/prod.html#src_hiir
[JACK]:         https://github.com/jackaudio/jack2
[libsndfile]:   https://github.com/erikd/libsndfile/
[LV2]:          https://lv2plug.in/
[our website]:  https://sfz.tools/sfizz
[AppVeyor Build Status]: https://img.shields.io/appveyor/ci/sfztools/sfizz.svg?label=Windows&style=popout&logo=appveyor
[Travis Build Status]:   https://img.shields.io/travis/com/sfztools/sfizz.svg?label=Linux&style=popout&logo=travis
