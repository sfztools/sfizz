# sfizz

[![Travis Build Status]](https://travis-ci.com/sfztools/sfizz)

SFZ library and LV2 plugin, please check [our website] for more details.

![Screenshot](screenshot.png)

## Author and contributors

Contributors to `sfizz` include:
- Paul Ferrand (2019-2020) (maintainer)
- Andrea Zanellato (2019-2020) (devops, documentation and distribution)
- Jean-Pierre Cimalando (2020)
- Michael Willis (2020)
- Alexander Mitchell (2020)

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
- `neon_mathfun.h` and `sse_mathfun.h` by Julien Pommier,
  licensed under the zlib license

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
[Travis Build Status]: https://img.shields.io/travis/com/sfztools/sfizz.svg?label=Linux&style=popout&logo=travis
