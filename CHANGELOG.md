# Changelog

The format is based on [Keep a Changelog][1].

This project tries to adhere to [Semantic Versioning][2].

## [Unreleased]

## [1.2.1] - 2023-04-15

### Globally

#### Added

- Mac OS Installer (#93 #1087)
- This file from Release-Notes.md in wiki.

#### Changed

- Updated bundled dependencies and added more options to use those
  preinstalled in system (#1104 #1117 #1143 #1145 #1149)
- CI: Various updates and fixes for GitHub Actions; removed Travis and Appveyor

### Library

#### Added

- Add LFO frequency as an EG target (#1103)
- New `image_controls` opcode to add a background image on UI' Controls tab (#1156)

#### Fixed

- Disabled temporarily the embedded sample test
- Parse 0-valued note ons as note offs (#1072 #1073)
- Correct a bug with dynamic updates on offed EGs (#2 #1088)
- jack: Wait for CLI thread to finish on quit (#1097)
- Fix libsndfile build and add CI (#1112 #1090)
- Last keyswitches don't change the midi state (#1113 #1074)
- Add section suffix to man pages (#1124)

### UI

#### Added

- HiDPI: Added a Zoom menu in Settings to apply an user' zoom ratio preference
  (#1123 #1153)

#### Changed

- Colored logo button on mouse hover (#1151)
- Centered, full-size background images container (#1152)
- XP style tooltips (#1156)
- Window size has been changed to be compatible with ARIA banks
  (775x335 for image backgrounds) (#1140 #1156)
- The CC knobs rotator and title label have a white color
  on a semi-transparent background in the controls tab (#1156)
- The info tab becomes a dark and semi-transparent overlay dialog above the
  Home tab; both will display the same background image when available (#1156)

### Plugins

#### Added

- Handle LV2 scaleFactor supportedOption (#1153)

#### Fixed

- Added suffix "-multi" to LV2 and VST3. (#1084)
- Fix LV2 manifest when using PSA (#1114)
- Fix 2 bugs in LV2 and VST3 for loads and stores (#1115 #1098)

## [1.2.0] - 2022-02-15

### Major improvements

- The sfizz engine now handles multiple stereo outputs,
  through the `output` opcode. The VST3 and LV2 plugins both have
  a 16-out version (as 8 stereo outs) (#1002)
- The `emscripten` branch allows to run sfizz through WASM
  for your web-oriented needs. An example with WebMidi
  is available at https://sfz.tools/sfizz-webaudio/
  (requires a Chromium-based browser),
  with source code at https://github.com/sfztools/sfizz-webaudio.
- sfizz can now parse embedded samples in base64 data (#1041)

### Library

#### Added

- Added `sfizz_send_program_change` and its C++ counterpart.
- Added `sfizz_set_sustain_cancels_release` and its C++ counterpart.
- Support extended CCs 140 and 141 for "keydelta" (#1003)
- Added better support for files that are fully loaded in memory
  and not simply preloaded (#1031)
- Added support for `loprog/hiprog` (#1058)

#### Changed

- The internal midi state is not reset when loading/reloading.
  This means in particular that controls changed on an instrument
  will be kept if you edit the underlying SFZ file (#1002)
- Updated dependent libraries (#1018)
- Negative values for lokey will not disable the region anymore (#1019)
- Choke groups now act through CC switches (#1024)
- `sw_last` is now properly affected by `octave_offset` and `note_offset` (#1039)
- sfizz is now clamping flex EG levels (#1037)

#### Fixed

- Corrected a bug when reloading some files (#1028)
- Corrected mistakes in the shelving filters (#1027)
- Properly read exponential notation in floats (#1036)
- Corrected a bug where long-tailed effects would be cut off (#1050)
- Corrected a bug in the band rejection filters (#1052).
  The `apf_1p` filter still misbehaves and is disabled for now.

#### Deprecated

- `sfizz_enable_logging` and `sfizz_disable_logging`
  (and their C++ counterparts) are deprecated,
  replaced by `sfizz_get_callback_breakdown` which needs to be called
  at the end of a callback by the host (#1034).
  All file writing is handled by the host for logging.

### UI

#### Added

- Added support in the plugin GUI/parameters for the "sustain cancels release"
  behavior of LinuxSampler.

#### Changed

- Updated the GUI option panel for the plugins (#1022)
  and some hints on the tab buttons (#1057)

#### Fixed

- Numpad input is now properly handled (#1053)

### Plugins

#### Added

- In the LV2 plugin, plugin-side automation is now enabled by default
  (which means all SFZ control changes are naturally bound
  to incoming MIDI control changes and not controlled by a plugin parameter).
  You can disable this at build time and show the relevant parameters
  using `SFIZZ_LV2_PSA`. Too many hosts expect their plugins to respond
  to midi messages for things like pedal and volume. (#1054)
- The LV2 plugin now has a string output parameter
  with the CC used by the instrument (#1060)

#### Fixed

- Corrected a crash in some VST3 hosts (#1035)

## [1.1.1] - 2021-11-08 [YANKED]

### Fixed

- Roll back VSTGUI to 4.10.2 to avoid corruption of the UI in some scenarios

## [1.1.0] - 2021-10-23

### Library

#### Added

- Added support for `hint_sustain_cancels_release`, which when activated
  reproduces a LinuxSampler behavior by sustaining already released notes (#898).
  A GUI option should come later.
- Added support for `amp_veltrack_oncc`, `pitch_veltrack_oncc`,
  and `fil_veltrack_oncc` (#938)
- Dynamic EGs are supported for flex EGs (#933 #932 #930)
- Support `_curvecc` for extended CCs (#993)
- Support `lorand` and `hirand` on CC-triggered regions (#997)
- The engine now parses `oscillator=auto` and only enables oscillators
  on short samples with wavetable tags in the WAV metadata (#954)
- Added a polyphony option to `sfizz_render` (#992)
- Added a text interface to `sfizz_jack` (#973)

#### Changed

- Filter resonance can now be negative (#919)
- The JACK and PureData clients can use custom importers (#926)
- Polyphony groups can have negative indices (#934)
- Parse `lokey=-1` as `hikey=-1` (#941)
- Since the default build of the LV2 plugin is not statically linked against
  libsndfile, the distribution license is changed to the ISC license (#943)

#### Fixed

- Fixed a bug which could prevent parameters to be automated in Ardour (#893)
- Fixed a bug where high definition CC,
  crossfade or aftertouch events could be ignored (#855 #896)
- Resetting all controllers will now reset them to their default,
  instead of 0 (#905)
- Fix `off_by` behavior with long-releasing notes (#972)
- Improved the preloading for monolithic wave files (#935)
- The DecentSampler importer now better translates the loop modes (#981)
- CC-triggered voices can be cut off by polyphony groups (#917)
- Allow building on MacOS 10.9 (#990)
- Corrected a bug where octave and note offset
  were not reset upon loading a new file (#994)
- Corrected a bug where CC-triggered voices could generate note-offs
  when cut by a polyphony group (#998)
- Added man pages for `sfizz_render` and `sfizz_jack`, plus jack client fix (#999)
- Various build system improvements for different platforms
  (#957 #958 #964 #967 #968 #989 #978 #942 #921 #920 #902 #849 #850 #856 #1001)

### UI

#### Added

- Added a GUI volume meter (#859)
- It is now possible to enter values for CCs on the plugin GUI
  by double clicking on the CC knob (#945).
  A high-resolution input option is also available on a right click.

#### Fixed

- Respect the editor set for sfz files on MacOS (#925)

### Plugins

#### Added

- Added a Puredata plugin (#869 #873 #874)
- Implement support for VST3 keyswitch and midi note names (#877 #876)

#### Fixed

- Corrected a bug for the LV2 plugin when used in the Ardour DAW (#924)
- Even when plugin-side automation is disabled (through `SFIZZ_LV2_PSA`),
  sustain and sostenuto CC events will still go through to the plugin to improve
  the user experience on hosts that do not support automatic mapping of control
  ports to midi events through the LV2 `midi:binding` extension (#963).
- Fixed a bug that could hang the background thread in the VST plugin
  (#984 #865 #864)
- The current keyswitch is properly saved and restored with the plugin state (#961)
- Improved the X11 runloop (#986)
- Corrected a bug which prevented VST3 or LV2 plugins
  to be loaded by the Ardour DAW (#995)

## [1.0.0] - 2021-04-16

- SFZ v1 is virtually supported except for a handful of opcodes.
  Please check https://sfz.tools/sfizz/development/status/
  for the up-to-date status of opcode support.
- It is now possible to build sfizz without relying on libsndfile,
  using a set of libraries. This is now the default build mode.
  Building with libsndfile can be enabled at configure time.
- The library and plugins can now load DecentSampler files,
  and could accomodate other formats.
- CCs, keyswitch range, key ranges and active keyswitch
  are now displayed in the editor for all plugins.
  There has been a lot of UI work to make it more practical to use.
- There is an OSC interface in the library, which allows to have introspection
  into the currently loaded file, the state of CCs/keyswitches,
  and also set some parameters for loaded regions.

### Library

#### Added

- Added documentation in the API calls to make clear
  that sample-accurate triggering of voice requires
  messages to be sent in order. The VST plugin in particular
  did not respect this and has been updated.
- There are new HD (float) versions of API calls for note and CC events.
- Added an info panel in the plugin UI
  (#793 #792 #791 #789 #788 #787 #782 #779 #773)
- Support DecentSampler and a generic importer for other formats (#725 #715 #680)
- Add high-definition versions of the API calls (#820)
- Support polyphonic aftertouch events, triggers and targets (#809 #764)
- Support monophonic aftertouch events, triggers and targets (#765 #630 )
- It is now possible to change some parameters of the regions
  after loading the file through the OSC interface (#761)
- Added modulation targets for amplitude, pan, width, position, pitch and volume (#760)
- Support CC modifiers for `end`, `loop_start` and `loop_end` (#752)
- Support CC modifiers for LFO targets (#753)
- Support SFZ v1 extended CCs (#747)
- Implement the sostenuto logic (#739)
- Added CC modulation for `depth` targets (#730)
- Implement v1 LFOs (#693)
- Support `count` and `loop_count` (#654)
- Support `delay_oncc` (#653)
- Support `sw_vel` (#650)
- Added windowed sinc interpolation for `sample_quality` from 3 to 10 (#609)
- Support `lfoN_beats` (#553)
- Add region and voice introspection to the OSC interface (#555)
- Add an alternative to libsndfile (#482 #534 #535 #536)
- Support disjoint keyswitch ranges, as well as `sw_lolast` and `sw_hilast` (#526)

#### Changed

- The ABI is broken, leading to a major version change.
- Thread-safety now has to be handled externally.
  The API has been updated to provide a concurrency spec.
- The current version of the internal oversampling factor was too unstable
  and has been disabled for the time being. The API and plugin parameters
  are staying for compatibility. A more robust implementation should come soon.
- A new `bpmTempo` method has been added to pass tempo as beats-per-minute
  rather than seconds per beat.
  The previous version is deprecated.
- `aftertouch` is renamed as `channel_aftertouch` (for C)
  and `channelAftertouch` (for C++) throughout the API
  to be more consistent with `polyAftertouch`.
- The C++ standard is set to 14 if you build the UI on LV2,
  or if you build the VST version. This will likely increase to 17 shortly
  due to the dependency on VSTGUI. The ui-less LV2 plugin and the library
  remain C++11 compatible.
  Note that windows builds already use C++17 on all targets. (#795)
- The library uses an Hermite interpolation by default (#828 #829)
- Corrected a bug where regions with long release envelopes
  would not stop on sample ending (#811)
- Update abseil and ghc::filesystem (#780), the tunings library from Surge
  (#785), catch2 (#711)
- Improve the choking logic to match the spec and other implementations (#778)
- Refactored opcode reading and storing (#748 #727 #721 #722 #720 #700 #559)
- Filter/EQ parameters are clamped even with modulation (#701 #699)
- Delay envelope start by the voice delay (#655 #432)
- Allow parsing of some badly formed files files (#600)
- Match ARIA's LFOs (#613)
- Improve the Hermite interpolation (#597)
- Enable fast-math on MSVC (#567)

#### Fixed

- Fix an error where sample indices could wrap for massive pitch modulations
  (#825 #751)
- Small files with only zeroes are considered as `*silence` (#831)
- Corrected a regression where sfizz would stop loading the SFZ file
  if some sample files cannot be loaded (#806)
- Fix the parser to support sharp (#) symbols in file names (#746)
- Improved the ADSR transitions from decay to sustain, and from release to 0 (#734 #373)
- Reset the smoothers if the playhead moved (#731 #710 #382)
- Corrected a bug where the first sample was ignored on playback (#732 #394)
- If the sustain level is low enough the envelope is set to free-run (#723, #676)
- Fix loop crossfades when the loop starts early in the sample (#718)
- Fix random generators to match ARIA's behavior (#719)
- Fix corruption appearing in some sample libraries (#717 #695)
- Smoothers are now linear (#713 #714 #703)
- Fix a bug where the sample rate was not propagated properly to the flex EGs (#683)
- Fix `note_polyphony` choking (#634 #529)
- Respect the `end` opcode (#618)
- Fix allocations that may happen on the RT thread (#621)
- Fix the polyphony manager when the engine polyphony is changed
  after a file is loaded (#619)
- Avoid reloading invalid files (#614)
- Update AtomicQueue to correct a bug (#583)
- Fix crashes when building with libsndfile (#542 #543)
- Properly pad and align the wav files upon reading (#538)
- Handle gracefully having a different `sw_default` in a region (#531)

### UI

#### Added

- UI improvements and tweaks, adding themability in particular (#826 #824 #822
  #821 #819 #818 #816 #812 #799 #743 #691 #674 #673 #672 #670 #662 #661 #656)
- Show the current keyswitch on the editor and the virtual keybnoard (#665 #657)
- Added a button to reset the scala file to default (#660)
- The plugins now display an image through the `image` opcode (#770)

#### Changed

- Improve font handling (#669 #622)
- Improve file handling in the UI (#645 #659 #658)
  and enable the use of a "default user directory" or environment variable.
  This also allows the plugin to "find" missing sfz files
  in a different environment (#644 #581 #572 #532)
- Update VST to 3.7.2 (#798)

### Plugins

#### Added

- Added the option to build a VST2 version of sfizz
  if you have the SDK available (#708)

#### Changed

- Order VST events (#810 #803)

#### Fixed

- Fix corruptions and crashes that happened in the LV2 plugin (#794 #668 #557)
- The internal controllers of sfizz are now presented as parameters
  in the LV2 plugin. Experimental midi binding is also active on them. By default
  the "direct" midi input is disabled and you need to map to the parameters.
  A compile-time option is available to re-enable the midi input. (#776)
- Fix crashes in the VST plugin (#709 #702 #681 #625)
- Enable checking if file changed even when the transport is not running (#712 #689)
- The Mac builds are now signed (#550 #552)

#### Removed

- Remove automation from un-automatable parameters in the VST plugin (#814)

## [0.5.1] - 2020-10-25 [YANKED]

### Fixed

- Corrected race conditions that appeared with the new thread and file pools
  (#507 #508 #514 #521)
- Take the internal oversampling factor into account for loop points,
  and solve an issue where loop points specified in sfz files
  were not taken into account (#506)
- Fix an implementation error for the internal hash function
  when applied on a single byte (#512)
- Knobs are linear in the AU plugin (#517)
- Fix a crash in VSTGUI (#520)
- Fix the resource path in the LV2 plugin under windows (#524)
- Add MacOS make install rules (#525)

## [0.5.0] - 2020-10-15

### Major improvements

#### Added

- Added basic support for Flex EGs (#388) as modulation sources (targets to come)
- Added basic support for LFOs (#338)  as modulation sources (targets to come)
- EGs and LFOs can now target EQs and filters (#424)
- A new GUI has been added and is common to the LV2 and VST plugin
  (#397 #404 #419 #489 #492 #495 #496 #497)
  still mostly work in progress, more to come!
- Provided build systems to use sfizz with the VCV-Rack SDK
  and the DISTRHO Plugin Framework

### Library

#### Added

- Added API support for setting the playback state,
  time position and signature (#354)
- The API documentation on the sfizz's website has been streamlined alot !
- Added support for `sustain_lo` (#327)
- Audio files are now read incrementally, improving the availability under load (#294)
- Added support for effect types `reverb`, `disto`, `gate` and `comp`
- Added support for `off_time` and complete support for `off_mode`.
  The voice stealing logic was improved to take into account `polyphony`,
  `note_polyphony` and `group_polyphony` properly (#349 #352 #393 #413 #414 #467).
  Note that this support is also available for the engine polyphony.
  In this case, some additional voice will take over for the release duration (#477).
- Support for `offset_cc` (#385)
- `sfizz_render` now supports a `--quality` switch,
  which acts like the `sample_quality` opcode (#408)
- `pitch_keycenter=sample` is now taken into account (#362)
- Support `oscillator_detunecc` (#434)
- Support basic FM synthesis for oscillator regions (#436)
- Support `hint_ram_loading` for loading the whole samples in RAM (#353)
- Support for `loop_crossfade` (#464)
- Improve the filter shortcut path (#336)

#### Changed

- The voice stealing is now configurable using `hint_stealing`,
  with possible values as `first`, `oldest` (default), and `envelope_and_age`.
  The latter is the more CPU-consuming version which requires to follow
  the envelope of each voice to kill low-volume ones preferably.
  Note that the voice stealing continue to kill all voices
  started on the same event by default (i.e. all layers of the same note). (#344 #384 #353)
- `sfizz` now internally uses a modulation matrix to link
  all modulation sources (CC, LFOs, and EGs) and targets (#335 #351 #386)
- The wavetable quality has been improved (#347)
- Loading probable wavetable files, or wav files containing wavetable metadata
  now sets `oscillator=on` on the region (#431)
- The default `sample_quality` was put back to 1 for live playing and 2 for freewheeling (#405)
- Improve the file loading logic to keep files in memory
  for a short while in case they get reused (#425)
- Improved the release logic in many cases (#324 #414 #413)
- Set the level of the `*noise` generator to match ARIA's (#429)

#### Fixed

- CC 7, 10 and 11 are now linked by default to pan, volume and expression (#475)
- All phase-related opcodes in sfizz now use the 0->1 convention,
  as does ARIA/Sforzando, instead of the 0->360 convention (#499)
- Fix an unwanted copy in the realtime thread (#334)
- Fix the default `ampeg_attack` and `ampeg_release` to avoid clicks (#437)
- Corrected a race condition in freewheeling mode (#500)
- Fixed a potential non-realtime operation in the realtime thread (#498)
- Fix a bug when using a larger internal oversampling for regions
  with an `offset` value (#469)
- Fix an issue when loops occured more than once in a block (#462)
- Increase the range of the clamping on amplitude (#468) and pitch (#474)
- Fix CC modulations with their source depth set to 0 (#475)
- Fix an overshoot for crazily large cutoff values (#478); cutoffs are now clamped
- Fix the MIDNAM output for the case where extended CCs are used (#420)
- Fixed a bug where release voices where not ignored on self-mask search (#348)
- Fixed `amp_veltrack` behavior (#371)
- Fix the ADSRH envelope release rate (#376)
- Fixed an error for files where the loop spans the entire file (#378)
- Fixed `sustain_cc` behavior (#377)
- Match the default volumes with ARIA (#381)
- Properly set the `loop_mode` for release regions (#379)
- Regions with `end=0` are now properly disabled (#380)
- Fix `fil_random` to be bipolar (#452)
- The `sequence` order now properly starts at 1 (#451)
- Fix an issue on Flush to Zero on some ARM platforms (#455)
- Fix `pitch_veltrack` (#461)
- Opcode values now properly stop at the `<` character (#439)
- Fix various build errors and issues on all platforms
  (#345 #401 #400 #399 #417 #447 #449 #443 #453 #456 #459 #471 #484 #487 #488 #491)
- Existing and known CC values are now correctly taken into account for modulations (#421)
- Fix various performance regressions and improved performance
  especially on ARM builds (#410 #412 #415 #426)

### UI

#### Added

- The file dialog initial directory is now the root of the current loaded file (#428)

### Plugins

#### Added

- A new output port for active voices has been added in the LV2 plugin (#321)

#### Fixed

- Support for `atom:Blank` atoms in the LV2 plugin (#363)

## [0.4.0] - 2020-07-24

### Major improvements

#### Added

- Added support for polynomial resamples and `sample_quality` opcodes (#238 #267).
  The engine now defaults to a value of `2` for this opcode,
  which is more intensive than the original linear interpolation resampler
  but provides a better quality. Added support for better resampling algorithms
  also in the wavetables via `oscillator_quality` (#287).
- Support `_curvecc` and `_stepcc` opcodes (#166 #155 #77)
  as well as `_smoothcc` opcodes (#181 #48 #22 #153 #297 #285)
- Added support and API for Scala tuning files in the engine and the plugins
  (#253 #268 #282)

### Library

#### Added

- Added API calls to set `$variable` define values prior to loading an SFZ file
  (#168 #119 #130)
- Added API calls to get key labels and cc labels
  defined by `label_key` and `label_cc` (#174)
- Added an API call to load an sfz file as an `std::string` or `const char*` (#217)
- Added API calls for Scala files and tunings (#253)
- Added high-definition floating point CC API calls (#244)
- Added API calls to change the default resampling quality (#267 #238)
- Added support for unison oscillators (#161)
- Support for the `polyphony` opcode at all levels (#171 #275),
  as well as `note_polyphony`. The `group=` polyphony is also more flexible
  and can be defined anywhere.
- Added support for `offset_cc` (#170 #159)
- Added support for `direction=reverse` (#185 #179)
- Added support to label the keys using a `label_key` opcode.
  This is not really standard yet, but it is now integrated in the LV2 plugin
  to advertise the names in the MIDNAM file and possibly change their labels
  in hosts that support it. (#174 #154)
- Added support for block comments `/* */` in the parser (#196 #195)
- Added a `sfizz_render` client in tree; you can build it with the make target
  `sfizz_render` if the `SFIZZ_RENDER` CMake variable is set to `ON`. (#200 #201 #206)
- Add support to integrate sfizz in DPF plugins (#216)
- Added support for the `set_hdcc` opcodes and overall added the ability
  to support floating-point CCs from the API (#233 #232 #244)
- Added support for FLAC loops (#242 #229)
- Add headers and group sources in the CMake project for integration with e.g. Qt (#312)
- Support flat notes parsed as string values (#291 #289)

#### Fixed

- Solved some issues with DSmolken's drumkits related to the ampeg envelope (#172)
- An exception problem was thrown if an sfz file was deleted (#182 #184)
- Improved the filter stability (#198 #199 #210)
- Handle `USE_LIBCPP` properly on configure (#203)
- Fix the handling of loop markers if sample `end=` is present (#202 #204)
- Handle note on with 0 velocity as note offs in the jack client (#208 #211)
- Solved an issue with super short files (#215)
- Fixed some parsing issues with `$variables` (#230)
- Process `$` expansions in `#include` (#247)
- Change the default build type to `RelWithDebInfo` (#249)
- Improve the note stealing algorithm (#214);
  note that this is still very much a work in progress since many heuristics
  are in play here. Feel free to report misbehavior
  regarding note stealing as we improve this regularly.
- Corrected a bug with SFZ v1 `velcurve` (#263)
- Properly support the `off_by=-1` opcode to correctly reset the value. (#235)
- Ignore garbage values following e.g. a key number in opcode values
  (as in `key=64Garbage` -> `key=64`) (#263)
- `ampeg_****_onccXX` modifiers now properly consider multiple CC modifiers
  (#300 #167)
- Trigger on CC does not require disabling the key triggering through e.g. `key=-1` (#315)
- Improved handling of `release_key` (#298); still not perfect, if the region
  spans multiple key and multiple notes happened with the pedal down,
  only a single voice will start.
- Corrected a parsing issue when `$variables` were part of an opcode name (#328)
- Various other plumbing changes

### Plugins

#### Added

- Added an AudioUnit target (#224)
- Support the `mapPath` feature of the LV2 specifications,
  for tentatively better portability in plugin states (#303)
- New instances of the sfizz LV2 plugin will now load a default
  `*sine` instrument (#283)

#### Fixed

- Properly bundle the `dylib` for macOS (#188)
- Corrected a stack smashing bug in the LV2 plugin (#226)
- Properly advertise the VST plugin parameters (#241)
- Corrected some errors with null-terminated atoms in the LV2 plugin (#269)
- Properly read the LV2 option list until the end (#323, by @atsushieno)

## [0.3.2] - 2020-04-03

### Added

- Added an experimental support for `make uninstall` (#118, #120)
- Add the autopan (#105), width, rectifier, gain, limiter (#131),
  and string resonator (#143) effects
- Curves are now registered within the synth but cannot be referenced yet (#96)
- Added a "panic button" API that kills voices (#122)
- Added support for more generators using wavetables (#61)
- Added support for the `oscillator` opcode, to create generators from files (#128)
- Added support for `note_polyphony`, `polyphony`, and `note_selfmask` (#142)
- Added support for `pitch_cc` and `tune_cc` modifiers (#142)

### Changed

- sfizz now builds down to gcc-4.9 with stricter C++11 compliance.
  The main release builds use C++17 mode on newer compilers (#111, #110)
- Upstream libraries updates (abseil, filesystem and atomic_queue) (#121)
- The logging API can be used to set a log filename (a6cbb48)
- Reworked the parser; the new one is more efficient,
  and can indicate error/warning ranges (#130)
- The VST plugin now reloads the file automatically, like the LV2 plugin (#139)
- The max number of CCs was increased to 512,
  to accomodate some libraries that use cc300 modifiers.
- The engine uses floating point values internally for midi events (#137);
  this prepares it for high-resolution midi down the line.
- The modifier support was overhauled;
  all regions can now have multiple CCs modifying the same target (#142).
- Improved performance of the amplitude stage gain of the rendering process (#145)
- The VST3 are now a submodule; more architecture targets have been added
  (#158, #147, patch proposed by @hexdump0815)

### Fixed

- Corrected a bug where the VST plugin got recreated needlessly in some hosts (#122)
- Corrected a potential overflow for CC names (930bfdf)
- Generators using wavetables are now correctly tuned (#126)
- The stereo panning stage of the process was corrected;
  width is now set to 100% by default as it should,
  and panning is properly applied (1faa7f, b55171, #133)
- Corrected errors in the performance report script related to display values
  (file names and histogram range)
- Fixes some realtime synchronization issues in the VST (#140)
- Corrected bugs and differences with Cakewalk/ARIA in the ADSR envelope (#136, #129)

## [0.3.1] - 2020-03-14

### Added

- Added a VST3 plug-in front-end to the library. It is still quite experimental
  and suffers from problems that stem from the VST3 SDK itself. (#99)
- Added effect buses and processing. There is a "lofi" effect available for now,
  as well as the same filters and EQs you can apply on the regions.
  More will come soon! (#84)
- Added a script to parse and render the timings.
  This can help tracking performance issues and regressions. (#89)

### Fixed

- Various fixups, performance improvements, and CI updates.

## [0.3.0] - 2020-02-29

### Added

- Added filter and EQ handling (the `filN_...` and `eqN_...` opcodes).
  There are also no limits to the amount of filters and EQs you can slap
  on each region beyond your CPU. Most if not all of the relevant filter types
  from the SFZ v2 spec are supported.
- Added a new command-line option for the JACK client
  to set the client's name (#75, #76).
- Added initial MIDNAM support (#79).
  The MIDNAM shows the named CCs in the SFZ file for now.
- Added fine timings within the callbacks for performance improvements
  and regression testing (#65).
- Added a crude `*noise` generator. This generator is a bit expensive
  for what it does but it's mostly useful to test the filters.

### Changed

- Reworked the parsing code for faster dispatching and better handling
  of complex opcodes with multiple parameters in their opcode name (#40).
- Reworked the panning and stereo image process.
  The new process uses tabulated functions and avoid expensive calls
  to compute sine and cosine functions (#47, #56).

### Fixed

- Corrected a bug with Ardour where saving a session with no file loaded
  would crash on reopening.
- Corrected a bug where voices triggered on key off would never end
  and fill up the polyphony (#63).
- Improved and completed CI on all platforms.

## [v0.2.0] - 2020-01-30

First version not depending on SFZero / JUCE,
becoming a library to be used in other projects, plus a LV2 plugin.

### Added

- Added an LV2 plugin version.
- Added support for pitch bends (#6)
  as well as pitch-bend activation for regions (`lobend` and `hibend` opcodes).
- Added dynamic updates for the current modifiers
  (panning, stereo image, volume and amplitude mainly) (#19, #28)
- Added timing for callbacks and file loading times.
- Added a windows build process for both the shared library and the LV2.
  `sfizz` now builds on all major platforms.

### Changed

- The parser now falls back to case-insensitive search if it doesn't find
  the sample file in its current path (#28),
  so that the behavior of SFZ libraries on case-sensitive filesystems
  will match Windows and macOS default case-insensitive filesystems.
- The file now reload automatically on file change,
  and you can force a reload if necessary (#17).

### Fixed

- Corrected a bug where memory would be read past the end of the file in memory,
  generating artifacts.
- Corrected a bug where the real-time queue handling background loading
  of the voices would fail spuriously.
- Corrected a bug where in the LV2 plugin the unknown opcode list was truncated (#18).
- The JACK client will warn you instead of crashing
  if you do not give it a file to load (#27).

## [v0.1.0] - 2019-05-30

- Initial release, based on SFZero and JUCE.


[1]: https://keepachangelog.com/en/1.0.0/
[2]: https://semver.org/spec/v2.0.0.html

[Unreleased]: https://github.com/sfztools/sfizz/compare/1.2.1...HEAD
[1.2.1]:  https://github.com/sfztools/sfizz/compare/1.2.0...1.2.1
[1.2.0]:  https://github.com/sfztools/sfizz/compare/1.1.1...1.2.0
[1.1.1]:  https://github.com/sfztools/sfizz/compare/1.1.0...1.1.1
[1.1.0]:  https://github.com/sfztools/sfizz/compare/1.0.0...1.1.0
[1.0.0]:  https://github.com/sfztools/sfizz/compare/0.5.1...1.0.0
[0.5.1]:  https://github.com/sfztools/sfizz/compare/0.5.0...0.5.1
[0.5.0]:  https://github.com/sfztools/sfizz/compare/0.4.0...0.5.0
[0.4.0]:  https://github.com/sfztools/sfizz/compare/0.3.2...0.4.0
[0.3.2]:  https://github.com/sfztools/sfizz/compare/0.3.1...0.3.2
[0.3.1]:  https://github.com/sfztools/sfizz/compare/0.3.0...0.3.1
[0.3.0]:  https://github.com/sfztools/sfizz/compare/v0.2.0...0.3.0
[v0.2.0]: https://github.com/sfztools/sfizz/commits/v0.2.0
[v0.1.0]: https://github.com/sfztools/sfizz-juce/releases/tag/v0.1.0-beta.2
