# sfizz

[![build actions]](https://github.com/sfztools/sfizz/actions)
[![build obs]](https://build.opensuse.org/package/show/home:sfztools:sfizz:develop/sfizz)

[![Discord Badge Image]](https://discord.gg/3ArE9Mw)
[![SFZv1 Status Image]](https://sfz.tools/sfizz/development/status/opcodes/?v=1)
[![SFZv2 Status Image]](https://sfz.tools/sfizz/development/status/opcodes/?v=2)
[![ARIA Status Image]](https://sfz.tools/sfizz/development/status/opcodes/?v=aria)
[![Cakewalk Status Image]](https://sfz.tools/sfizz/development/status/opcodes/?v=cakewalk)

SFZ parser and synth c++ library and JACK standalone client,
please check [our website] for more details, or [our wiki] for further information.

## Repository reorganization

> **Warning**
>
> On 2023/05/08 the project was divided into 2 repositories:
> this contains only the library required by third-party applications,
> and [sfizz-ui] for plugins (and a future possible graphics standalone).

[sfizz-ui]: https://github.com/sfztools/sfizz-ui

So if you are looking for plugins, you can get both by looking for the `sfizz-ui`
repository instead, which contains also this one in the `library` subdirectory/git submodule.

## Using sfizz

Sfizz can be used most easily within an LV2 host such as [Carla] or [Ardour].
It can also be integrated as a library within your own program; check out our [API] bindings for C and C++.
Our [releases] are an immediate way to get a working library for Windows and Mac.
Linux builds are available over at [OBS].
On any operating system, you might prefer to [build from source]!

## Contributing to sfizz

There is actually many things anyone can do, programming-related or music-related.
Please check out the [CONTRIBUTING] document for information about filing bug reports or feature requests,
and helping the development of sfizz

## Donating to sfizz

Sfizz and the work in the SFZ tools organization is purely driven by hobbyists
who choose to use their free time to benefit this project.
We firmly believe in the honesty and goodwill of users as a whole,
and we want to promote a healthy relationship to software and to the cost of producing quality software.

No financial returns is explicitely required from using sfizz in any shape.
However, if you feel that sfizz produces value for you or your products,
and if you find that your financial situation allows for it, we put together ways to donate to the project.
You are never compelled to do so, the [CONTRIBUTING] file contains different ways to contribute.

In all of sfizz's governance model, we strive to live in the open.
Finances are no different, and we put in place a process so that the use of donations
is as transparent as possible through our [Open Collective].
We invite you to check out the [GOVERNANCE] file to see how the organization is governed and how are donations handled.

## Dependencies and licenses

The sfizz library has the option to be compiled against either the `dr_libs`
audio libraries, which is now the default option, or `libsndfile`.

- [dr_libs] is licensed under the MIT No Attribution license
- [libsndfile] is licensed under the GNU Lesser General Public License v2.1

The sfizz library makes primary use of:

- [Abseil], licensed under the Apache License 2.0
- [atomic_queue] by Maxim Egorushkin, licensed under the MIT license
- [filesystem] by Steffen Schümann, licensed under the BSD 3-Clause license
- [hiir] by Laurent de Soras, licensed under the WTFPL v2 license
- [KISS FFT] by Mark Borgerding, licensed under the BSD 3-Clause license
- [Surge tuning] by Paul Walker, licensed under the MIT license
- [pugixml] by Arseny Kapoulkine, licensed under the MIT license
- [cephes] by Stephen Moshier, licensed under the BSD 3-Clause license
- [cpuid] by Steinwurf ApS, licensed under the BSD 3-Clause license
- [faust-libraries] by GRAME, Julius O. Smith III and Eric Tarr,
  licensed under the STK-4.3 license and a permissive variant of the LGPL license

The sfizz library also uses in some subprojects:

- [Catch2], licensed under the Boost Software License 1.0
- [benchmark], licensed under the Apache License 2.0
- [JACK], licensed under the GNU Lesser General Public License v2.1
- [cxxopts] by Jarryd Beck, licensed under the MIT license
- [fmidi] by Jean Pierre Cimalando, licensed under the Boost Software License 1.0
- [libsamplerate], licensed under the BSD 2-Clause license


[CONTRIBUTING]:          CONTRIBUTING.md
[GOVERNANCE]:            GOVERNANCE.md
[Abseil]:                https://abseil.io/
[atomic_queue]:          https://github.com/max0x7ba/atomic_queue/
[benchmark]:             https://github.com/google/benchmark/
[Catch2]:                https://github.com/catchorg/Catch2/
[filesystem]:            https://github.com/gulrak/filesystem/
[Surge tuning]:          https://surge-synth-team.org/tuning-library/
[pugixml]:               https://pugixml.org/
[cephes]:                https://www.netlib.org/cephes/
[cpuid]:                 https://github.com/steinwurf/cpuid/
[dr_libs]:               https://github.com/mackron/dr_libs/
[faust-libraries]:       https://github.com/grame-cncm/faustlibraries/
[hiir]:                  http://ldesoras.free.fr/prod.html#src_hiir
[KISS FFT]:              http://kissfft.sourceforge.net/
[JACK]:                  https://github.com/jackaudio/jack2/
[cxxopts]:               https://github.com/jarro2783/cxxopts/
[fmidi]:                 https://github.com/jpcima/fmidi/
[libsamplerate]:         http://www.mega-nerd.com/SRC/
[libsndfile]:            http://www.mega-nerd.com/libsndfile/
[our website]:           https://sfz.tools/sfizz/
[our wiki]:              https://sfz.tools/sfizz-wiki/
[releases]:              https://github.com/sfztools/sfizz/releases/
[Carla]:                 https://kx.studio/Applications:Carla
[Ardour]:                https://ardour.org/
[API]:                   https://sfz.tools/sfizz/api/
[Open Collective]:       https://opencollective.com/sfztools
[build from source]:     https://sfz.tools/sfizz/development/build/
[Discord Badge Image]:   https://img.shields.io/discord/587748534321807416?label=discord&logo=discord
[build actions]:         https://github.com/sfztools/sfizz/actions/workflows/build.yml/badge.svg?branch=develop
[build obs]:             https://build.opensuse.org/projects/home:sfztools:sfizz:develop/packages/sfizz/badge.svg
[OBS]:                   https://software.opensuse.org//download.html?project=home%3Asfztools%3Asfizz&package=sfizz
[SFZv1 Status Image]:    https://sfz.tools/assets/img/sfizz/badge_sfz1.svg
[SFZv2 Status Image]:    https://sfz.tools/assets/img/sfizz/badge_sfz2.svg
[ARIA Status Image]:     https://sfz.tools/assets/img/sfizz/badge_aria.svg
[Cakewalk Status Image]: https://sfz.tools/assets/img/sfizz/badge_cakewalk.svg

[AppVeyor Build Status]: https://img.shields.io/appveyor/ci/sfztools/sfizz.svg?label=Windows&style=popout&logo=appveyor
[Travis Build Status]:   https://img.shields.io/travis/com/sfztools/sfizz.svg?label=Linux&style=popout&logo=travis
