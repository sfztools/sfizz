### 1.2.0

The big stuff:
- The sfizz engine now handles multiple stereo outputs, through the `output` opcode. The VST3 and LV2 plugins both have a 16-out version (as 8 stereo outs) (#1002)
- The `emscripten` branch allows to run sfizz through WASM for your web-oriented needs. An example with WebMidi is available at https://sfz.tools/sfizz-webaudio/ (requires a Chromium-based browser), with source code at https://github.com/sfztools/sfizz-webaudio.
- sfizz can now parse embedded samples in base64 data (#1041)

Smaller things:
- Added support in the plugin GUI/parameters for the "sustain cancels release" behavior of LinuxSampler.
- The internal midi state is not reset when loading/reloading. This means in particular that controls changed on an instrument will be kept if you edit the underlying SFZ file (#1002)
- Updated dependent libraries (#1018)
- Support extended CCs 140 and 141 for "keydelta" (#1003)
- Negative values for lokey will not disable the region anymore (#1019)
- Updated the GUI option panel for the plugins (#1022) and some hints on the tab buttons (#1057)
- Choke groups now act through CC switches (#1024)
- Corrected a bug when reloading some files (#1028)
- Added better support for files that are fully loaded in memory and not simply preloaded (#1031)
- Corrected mistakes in the shelving filters (#1027)
- Corrected a crash in some VST3 hosts (#1035)
- Properly read exponential notation in floats (#1036)
- sfizz is now clamping flex EG levels (#1037)
- `sw_last` is now properly affected by `octave_offset` and `note_offset` (#1039)
- Corrected a bug where long-tailed effects would be cut off (#1050)
- Corrected a bug in the band rejection filters (#1052). The `apf_1p` filter still misbehaves and is disabled for now.
- Numpad input is now properly handled (#1053)
- In the LV2 plugin, plugin-side automation is now enabled by default (which means all SFZ control changes are naturally bound to incoming MIDI control changes and not controlled by a plugin parameter). You can disable this at build time and show the relevant parameters using `SFIZZ_LV2_PSA`. Too many hosts expect their plugins to respond to midi messages for things like pedal and volume. (#1054)
- Added support for `loprog/hiprog` (#1058)
- The LV2 plugin now has a string output parameter with the CC used by the instrument (#1060)

API changes:
- `sfizz_enable_logging` and `sfizz_disable_logging` (and their C++ counterparts) are deprecated, replaced by `sfizz_get_callback_breakdown` which needs to be called at the end of a callback by the host (#1034). All file writing is handled by the host for logging.
- Added `sfizz_send_program_change` and its C++ counterpart.
- Added `sfizz_set_sustain_cancels_release` and its C++ counterpart.

### 1.1.1
- Roll back VSTGUI to 4.10.2 to avoid corruption of the UI in some scenarios

### 1.1.0

New supports and plugins:
- Added a Puredata plugin (#869 #873 #874)
- Added support for `hint_sustain_cancels_release`, which when activated reproduces a LinuxSampler behavior by sustaining already released notes (#898). A GUI option should come later.
- Added support for `amp_veltrack_oncc`, `pitch_veltrack_oncc`, and `fil_veltrack_oncc` (#938)
- Dynamic EGs are supported for flex EGs (#933 #932 #930)
- Support `_curvecc` for extended CCs (#993)
- Support `lorand` and `hirand` on CC-triggered regions (#997)
- The engine now parses `oscillator=auto` and only enables oscillators on short samples with wavetable tags in the WAV metadata (#954)

Bug fixes and other improvements:
- Added a GUI volume meter (#859)
- Implement support for VST3 keyswitch and midi note names (#877 #876)
- Fixed a bug which could prevent parameters to be automated in Ardour (#893)
- Fixed a bug where high definition CC, crossfade or aftertouch events could be ignored (#855 #896)
- Resetting all controllers will now reset them to their default, instead of 0 (#905)
- Filter resonance can now be negative (#919)
- Corrected a bug for the LV2 plugin when used in the Ardour DAW (#924)
- Respect the editor set for sfz files on MacOS (#925)
- The JACK and PureData clients can use custom importers (#926)
- Fix `off_by` behavior with long-releasing notes (#972)
- Polyphony groups can have negative indices (#934)
- Parse `lokey=-1` as `hikey=-1` (#941)
- Improved the preloading for monolithic wave files (#935)
- Since the default build of the LV2 plugin is not statically linked against libsndfile, the distribution license is changed to the ISC license (#943)
- The DecentSampler importer now better translates the loop modes (#981)
- Even when plugin-side automation is disabled (through `SFIZZ_LV2_PSA`), sustain and sostenu CC events will still go through to the plugin to improve the user experience on hosts that do not support automatic mapping of control ports to midi events through the LV2 `midi:binding` extension (#963).
- Fixed a bug that could hang the background thread in the VST plugin (#984 #865 #864)
- The current keyswitch is properly saved and restored with the plugin state (#961)
- It is now possible to enter values for CCs on the plugin GUI by double clicking on the CC knob (#945). A high-resolution input option is also available on a right click.
- Added a text interface to `sfizz_jack` (#973)
- CC-triggered voices can be cut off by polyphony groups (#917)
- Allow building on MacOS 10.9 (#990)
- Improved the X11 runloop (#986)
- Added a polyphony option to `sfizz_render` (#992)
- Corrected a bug which prevented VST3 or LV2 plugins to be loaded by the Ardour DAW (#995)
- Corrected a bug where octave and note offset were not reset upon loading a new file (#994)
- Corrected a bug where CC-triggered voices could generate note-offs when cut by a polyphony group (#998)
- Added man pages for `sfizz_render` and `sfizz_jack` (#999)
- Various build system improvements for different platforms (#957 #958 #964 #967 #968 #989 #978 #942 #921 #920 #902 #849 #850 #856 #1001)

### 1.0.0

Headlines:
- SFZ v1 is virtually supported except for a handful of opcodes ! Please check
  https://sfz.tools/sfizz/development/status/ for the up-to-date status of opcode support.
- It is now possible to build sfizz without relying on libsndfile, using a set of libraries. This is now the
  default build mode. Building with libsndfile can be enabled at configure time.
- The library and plugins can now load DecentSampler files, and could accomodate other formats.
- CCs, keyswitch range, key ranges and active keyswitch are now displayed in the editor for all plugins. 
  There has been a lot of UI work to make it more practical to use.
- There is an OSC interface in the library, which allows to have introspection into the currently
  loaded file, the state of CCs/keyswitches, and also set some parameters for loaded regions.

API/ABI changes for the library:
- The ABI is broken, leading to a major version change.
- Thread-safety now has to be handled externally. The API has been updated to provide a concurrency spec.
- The current version of the internal oversampling factor was too unstable and has been disabled for the time
being. The API and plugin parameters are staying for compatibility. A more robust implementation should come soon.
- Added documentation in the API calls to make clear that sample-accurate triggering of voice requires
  messages to be sent in order. The VST plugin in particular did not respect this and has been updated.
- A new `bpmTempo` method has been added to pass tempo as beats-per-minute rather than seconds per beat.
  The previous version is deprecated.
- There are new HD (float) versions of API calls for note and CC events.
- `aftertouch` is renamed as `channel_aftertouch` (for C) and `channelAftertouch` (for C++) throughout the API
  to be more consistent with `polyAftertouch`.

UI:
- Fix an error where sample indices could wrap for massive pitch modulations (#825 #751)
- UI improvements and tweaks, adding themability in particular (#826 #824 #822 #821 #819 #818 #816 #812 #799 #743 #691 #674 #673 #672 #670 #662 #661 #656)
- Added an info panel in the plugin UI (#793 #792 #791 #789 #788 #787 #782 #779 #773)
- Improve font handling (#669 #622)
- Improve file handling in the UI (#645 #659 #658) and enable the use of a "default user directory" or environment variable.
  This also allows the plugin to "find" missing sfz files in a different environment (#644 #581 #572 #532)
- Support DecentSampler and a generic importer for other formats (#725 #715 #680)
- Show the current keyswitch on the editor and the virtual keybnoard (#665 #657)
- Added a button to reset the scala file to default (#660)
- The plugins now display an image through the `image` opcode (#770)

Plugins:
- Remove automation from un-automatable parameters in the VST plugin (#814)
- Order VST events (#810 #803)
- Added the option to build a VST2 version of sfizz if you have the SDK available (#708)
- The C++ standard is set to 14 if you build the UI on LV2, or if you build the VST version. This will likely
  increase to 17 shortly due to the dependency on VSTGUI. The ui-less LV2 plugin and the library remain C++11
  compatible. Note that windows builds already use C++17 on all targets. (#795)
- Fix corruptions and crashes that happened in the LV2 plugin (#794 #668 #557)
- The internal controllers of sfizz are now presented as parameters in the LV2 plugin. Experimental midi binding
  is also active on them. By default the "direct" midi input is disabled and you need to map to the parameters.
  A compile-time option is available to re-enable the midi input. (#776)
- Fix crashes in the VST plugin (#709 #702 #681 #625)
- Enable checking if file changed even when the transport is not running (#712 #689)
- The Mac builds are now signed (#550 #552)
- Update VST to 3.7.2 (#798)

Library:
- Small files with only zeroes are considered as `*silence` (#831)
- The library uses an Hermite interpolation by default (#828 #829)
- Add high-definition versions of the API calls (#820)
- Corrected a bug where regions with long release envelopes would not stop on sample ending (#811)
- Support polyphonic aftertouch events, triggers and targets (#809 #764)
- Support monophonic aftertouch events, triggers and targets (#765 #630 )
- Corrected a regression where sfizz would stop loading the SFZ file if some sample files cannot be loaded (#806)
- Update abseil and ghc::filesystem (#780), the tunings library from Surge (#785), catch2 (#711)
- It is now possible to change some parameters of the regions after loading the file through the OSC interface (#761)
- Improve the choking logic to match the spec and other implementations (#778)
- Added modulation targets for amplitude, pan, width, position, pitch and volume (#760)
- Support CC modifiers for `end`, `loop_start` and `loop_end` (#752)
- Support CC modifiers for LFO targets (#753)
- Support SFZ v1 extended CCs (#747)
- Implement the sostenuto logic (#739)
- Refactored opcode reading and storing (#748 #727 #721 #722 #720 #700 #559)
- Fix the parser to support sharp (#) symbols in file names (#746)
- Improved the ADSR transitions from decay to sustain, and from release to 0 (#734 #373)
- Reset the smoothers if the playhead moved (#731 #710 #382)
- Corrected a bug where the first sample was ignored on playback (#732 #394)
- Added CC modulation for `depth` targets (#730)
- If the sustain level is low enough the envelope is set to free-run (#723, #676)
- Fix loop crossfades when the loop starts early in the sample (#718)
- Fix random generators to match ARIA's behavior (#719)
- Fix corruption appearing in some sample libraries (#717 #695)
- Smoothers are now linear (#713 #714 #703)
- Filter/EQ parameters are clamped even with modulation (#701 #699)
- Implement v1 LFOs (#693)
- Support `count` and `loop_count` (#654)
- Fix a bug where the sample rate was not propagated properly to the flex EGs (#683)
- Delay envelope start by the voice delay (#655 #432)
- Support `delay_oncc` (#653)
- Support `sw_vel` (#650)
- Fix `note_polyphony` choking (#634 #529)
- Respect the `end` opcode (#618)
- Fix allocations that may happen on the RT thread (#621)
- Fix the polyphony manager when the engine polyphony is changed after a file is loaded (#619)
- Avoid reloading invalid files (#614)
- Allow parsing of some badly formed files files (#600)
- Match ARIA's LFOs (#613)
- Added windowed sinc interpolation for `sample_quality` from 3 to 10 (#609)
- Improve the Hermite interpolation (#597)
- Update AtomicQueue to correct a bug (#583)
- Support `lfoN_beats` (#553)
- Add region and voice introspection to the OSC interface (#555)
- Enable fast-math on MSVC (#567)
- Fix crashes when building with libsndfile (#542 #543)
- Properly pad and align the wav files upon reading (#538)
- Add an alternative to libsndfile (#482 #534 #535 #536)
- Handle gracefully having a different `sw_default` in a region (#531)
- Support disjoint keyswitch ranges, as well as `sw_lolast` and `sw_hilast` (#526)

### 0.5.1

- Corrected race conditions that appeared with the new thread and file pools (#507 #508 #514 #521)
- Take the internal oversampling factor into account for loop points, and solve an issue where loop points specified in sfz files were not taken into account (#506)
- Fix an implementation error for the internal hash function when applied on a single byte (#512)
- Knobs are linear in the AU plugin (#517)
- Fix a crash in VSTGUI (#520)
- Fix the resource path in the LV2 plugin under windows (#524)
- Add MacOS make install rules (#525)

### 0.5.0

Big stuff:
- Added basic support for Flex EGs (#388) as modulation sources (targets to come)
- Added basic support for LFOs (#338)  as modulation sources (targets to come)
- EGs and LFOs can now target EQs and filters (#424)
- A new GUI has been added and is common to the LV2 and VST plugin (#397 #404 #419 #489 #492 #495 #496 #497); still mostly work in progress, more to come!
- Provided build systems to use sfizz with the VCV-Rack SDK and the DISTRHO Plugin Framework

New features:
- Added support for `sustain_lo` (#327)
- Audio files are now read incrementally, improving the availability under load (#294)
- A new output port for active voices has been added in the LV2 plugin (#321)
- Added support for effect types `reverb`, `disto`, `gate` and `comp`
- The voice stealing is now configurable using `hint_stealing`, with possible values as `first`, `oldest` (default), and `envelope_and_age`. The latter is the more CPU-consuming version which requires to follow the envelope of each voice to kill low-volume ones preferably. Note that the voice stealing continue to kill all voices started on the same event by default (i.e. all layers of the same note). (#344 #384 #353)
- `sfizz` now internally uses a modulation matrix to link all modulation sources (CC, LFOs, and EGs) and targets (#335 #351 #386)
- Added support for `off_time` and complete support for `off_mode`. The voice stealing logic was improved to take into account `polyphony`, `note_polyphony` and `group_polyphony` properly (#349 #352 #393 #413 #414 #467). Note that this support is also available for the engine polyphony. In this case, some additional voice will take over for the release duration (#477).
- The wavetable quality has been improved (#347)
- Support for `offset_cc` (#385)
- `sfizz_render` now supports a `--quality` switch, which acts like the `sample_quality=` opcode (#408)
- `pitch_keycenter=sample` is now taken into account (#362)
- Support `oscillator_detunecc` (#434)
- Support basic FM synthesis for oscillator regions (#436)
- CC 7, 10 and 11 are now linked by default to pan, volume and expression (#475)
- Support `hint_ram_loading` for loading the whole samples in RAM (#353)
- Support for `loop_crossfade` (#464)
- All phase-related opcodes in sfizz now use the 0->1 convention, as does ARIA/Sforzando, instead of the 0->360 convention (#499)

Issues:
- Loading probable wavetable files, or wav files containing wavetable metadata now sets `oscillator=on` on the region (#431)
- The default `sample_quality` was put back to 1 for live playing and 2 for freewheeling (#405)
- Fix an unwanted copy in the realtime thread (#334)
- Improve the filter shortcut path (#336)
- Fix the default `ampeg_attack` and `ampeg_release` to avoid clicks (#437)
- Corrected a race condition in freewheeling mode (#500)
- Fixed a potential non-realtime operation in the realtime thread (#498)
- Fix a bug when using a larger internal oversampling for regions with an `offset` value (#469)
- Fix an issue when loops occured more than once in a block (#462)
- Increase the range of the clamping on amplitude (#468) and pitch (#474)
- Fix CC modulations with their source depth set to 0 (#475)
- Fix an overshoot for crazily large cutoff values (#478); cutoffs are now clamped 
- Improve the file loading logic to keep files in memory for a short while in case they get reused (#425)
- Fix the MIDNAM output for the case where extended CCs are used (#420)
- Fixed a bug where release voices where not ignored on self-mask search (#348)
- Improved the release logic in many cases (#324 #414 #413)
- Set the level of the `*noise` generator to match ARIA's (#429)
- Support for `atom:Blank` atoms in the LV2 plugin (#363)
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
- Fix various build errors and issues on all platforms (#345 #401 #400 #399 #417 #447 #449 #443 #453 #456 #459 #471 #484 #487 #488 #491)
- The file dialog initial directory is now the root of the current loaded file (#428)
- Existing and known CC values are now correctly taken into account for modulations (#421)
- Fix various performance regressions and improved performance especially on ARM builds (#410 #412 #415 #426)

API changes:
- Added API support for setting the playback state, time position and signature (#354)
- The API documentation on the sfizz's website has been streamlined alot !

### 0.4.0

Big stuff:
- Added support for polynomial resamples and `sample_quality` opcodes (#238 #267). The engine now defaults to a value of `2` for this opcode, which is more intensive than the original linear interpolation resampler but provides a better quality. Added support for better resampling algorithms also in the wavetables via `oscillator_quality` (#287).
- Support `_curvecc` and `_stepcc` opcodes (#166 #155 #77) as well as `_smoothcc` opcodes (#181 #48 #22 #153 #297 #285)
- Added support and API for Scala tuning files in the engine and the plugins (#253 #268 #282)

Other new features:
- Added support for unison oscillators (#161)
- Support for the `polyphony` opcode at all levels (#171 #275), as well as `note_polyphony`. The `group=` polyphony is also more flexible and can be defined anywhere.
- Added support for `offset_cc` (#170 #159)
- Added support for `direction=reverse` (#185 #179)
- Added support to label the keys using a `label_key` opcode. This is not really standard yet, but it is now integrated in the LV2 plugin to advertise the names in the MIDNAM file and possibly change their labels in hosts that support it. (#174 #154)
- Added support for block comments `/* */` in the parser (#196 #195)
- Added a `sfizz_render` client in tree; you can build it with the make target `sfizz_render` if the `SFIZZ_RENDER` CMake variable is set to `ON`. (#200 #201 #206)
- Add support to integrate sfizz in DPF plugins (#216)
- Added an AudioUnit target (#224)
- Added support for the `set_hdcc` opcodes and overall added the ability to support floating-point CCs from the API (#233 #232 #244)
- Added support for FLAC loops (#242 #229)
- Support the `mapPath` feature of the LV2 specifications, for tentatively better portability in plugin states (#303)
- New instances of the sfizz LV2 plugin will now load a default `*sine` instrument (#283)

Issues:
- Solved some issues with DSmolken's drumkits related to the ampeg envelope (#172)
- An exception problem was thrown if an sfz file was deleted (#182 #184)
- Properly bundle the `dylib` for macOS (#188)
- Improved the filter stability (#198 #199 #210)
- Handle `USE_LIBCPP` properly on configure (#203)
- Fix the handling of loop markers if sample `end=` is present (#202 #204)
- Handle note on with 0 velocity as note offs in the jack client (#208 #211)
- Solved an issue with super short files (#215)
- Corrected a stack smashing bug in the LV2 plugin (#226)
- Fixed some parsing issues with `$variables` (#230)
- Properly advertise the VST plugin parameters (#241)
- Process `$` expansions in `#include` (#247)
- Change the default build type to `RelWithDebInfo` (#249)
- Improve the note stealing algorithm (#214); note that this is still very much a work in progress since many heuristics are in play here. Feel free to report misbehavior regarding note stealing as we improve this regularly.
- Corrected a bug with SFZ v1 `velcurve` (#263)
- Properly support the `off_by=-1` opcode to correctly reset the value. (#235)
- Corrected some errors with null-terminated atoms in the LV2 plugin (#269)
- Ignore garbage values following e.g. a key number in opcode values (as in `key=64Garbage` -> `key=64`) (#263)
- `ampeg_****_onccXX` modifiers now properly consider multiple CC modifiers (#300 #167) 
- Add headers and group sources in the CMake project for integration with e.g. Qt (#312)
- Trigger on CC does not require disabling the key triggering through e.g. `key=-1` (#315)
- Support flat notes parsed as string values (#291 #289)
- Improved handling of `release_key` (#298); still not perfect, if the region spans multiple key and multiple notes happened with the pedal down, only a single voice will start.
- Properly read the LV2 option list until the end (#323, by @atsushieno)
- Corrected a parsing issue when `$variables` were part of an opcode name (#328)
- Various other plumbing changes
 
API additions:
- Added API calls to set `$variable` define values prior to loading an SFZ file (#168 #119 #130)
- Added API calls to get key labels and cc labels defined by `label_key` and `label_cc` (#174)
- Added an API call to load an sfz file as an `std::string` or `const char*` (#217)
- Added API calls for Scala files and tunings (#253)
- Added high-definition floating point CC API calls (#244)
- Added API calls to change the default resampling quality (#267 #238)

### 0.3.2

- sfizz now builds down to gcc-4.9 with stricter C++11 compliance. The main release builds use C++17 mode on newer compilers (#111, #110)
- Upstream libraries updates (abseil, filesystem and atomic_queue) (#121)
- Added an experimental support for `make uninstall` (#118, #120)
- Add the autopan (#105), width, rectifier, gain, limiter (#131), and string resonator (#143) effects
- Curves are now registered within the synth but cannot be referenced yet (#96)
- Corrected a bug where the VST plugin got recreated needlessly in some hosts (#122)
- Added a "panic button" API that kills voices (#122)
- Corrected a potential overflow for CC names (930bfdf)
- Added support for more generators using wavetables (#61)
- Added support for the `oscillator` opcode, to create generators from files (#128)
- Generators using wavetables are now correctly tuned (#126)
- The stereo panning stage of the process was corrected; width is now set to 100% by default as it should, and panning is properly applied (1faa7f, b55171, #133)
- The logging API can be used to set a log filename (a6cbb48)
- Corrected errors in the performance report script related to display values (file names and histogram range)
- Reworked the parser; the new one is more efficient, and can indicate error/warning ranges (#130)
- The VST plugin now reloads the file automatically, like the LV2 plugin (#139)
- The max number of CCs was increased to 512, to accomodate some libraries that use cc300 modifiers.
- The engine uses floating point values internally for midi events (#137); this prepares it for high-resolution midi down the line.
- Fixes some realtime synchronization issues in the VST (#140)
- Added support for `note_polyphony`, `polyphony`, and `note_selfmask` (#142)
- Added support for `pitch_cc` and `tune_cc` modifiers (#142)
- The modifier support was overhauled; all regions can now have multiple CCs modifying the same target (#142).
- Corrected bugs and differences with Cakewalk/ARIA in the ADSR envelope (#136, #129)
- Improved performance of the amplitude stage gain of the rendering process (#145)
- The VST3 are now a submodule; more architecture targets have been added (#158, #147, patch proposed by @hexdump0815)

### 0.3.1

- Added a VST3 plug-in front-end to the library. It is still quite experimental and suffers from problems that stem from the VST3 SDK itself. (#99)
- Added effect buses and processing. There is a "lofi" effect available for now, as well as the same filters and EQs you can apply on the regions. More will come soon! (#84)
- Added a script to parse and render the timings. This can help tracking performance issues and regressions. (#89)
- Various fixups, performance improvements, and CI updates.

### 0.3.0

- Added filter and EQ handling (the `filN_...` and `eqN_...` opcodes). There are also no limits to the amount of filters and EQs you can slap on each region beyond your CPU. Most if not all of the relevant filter types from the SFZ v2 spec are supported.
- Added a new command-line option for the JACK client to set the client's name (#75, #76).
- Added initial MIDNAM support (#79). The MIDNAM shows the named CCs in the SFZ file for now.
- Reworked the parsing code for faster dispatching and better handling of complex opcodes with multiple parameters in their opcode name (#40).
- Reworked the panning and stereo image process. The new process uses tabulated functions and avoid expensive calls to compute sine and cosine functions (#47, #56).
- Added a crude `*noise` generator. This generator is a bit expensive for what it does but it's mostly useful to test the filters.
- Added fine timings within the callbacks for performance improvements and regression testing (#65).
- Corrected a bug with Ardour where saving a session with no file loaded would crash on reopening.
- Corrected a bug where voices triggered on key off would never end and fill up the polyphony (#63).
- Improved and completed CI on all platforms.

### 0.2.0

- Added an LV2 plugin version.
- The parser now falls back to case-insensitive search if it doesn't find the sample file in its current path (#28), so that the behavior of SFZ libraries on case-sensitive filesystems will match Windows and macOS default case-insensitive filesystems.
- The file now reload automatically on file change, and you can force a reload if necessary (#17).
- Corrected a bug where memory would be read past the end of the file in memory, generating artifacts.
- Corrected a bug where the real-time queue handling background loading of the voices would fail spuriously.
- Corrected a bug where in the LV2 plugin the unknown opcode list was truncated (#18).
- Added dynamic updates for the current modifiers (panning, stereo image, volume and amplitude mainly) (#19, #28)
- Added timing for callbacks and file loading times.
- Added support for pitch bends (#6) as well as pitch-bend activation for regions (`lobend` and `hibend` opcodes).
- The JACK client will warn you instead of crashing if you do not give it a file to load (#27).
- Added a windows build process for both the shared library and the LV2. `sfizz` now builds on all major platforms.

### 0.1.0

- Initial release