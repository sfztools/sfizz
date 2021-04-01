// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/modulations/ModId.h"
#include "sfizz/modulations/ModKey.h"
#include "sfizz/Synth.h"
#include "TestHelpers.h"
#include "catch2/catch.hpp"

TEST_CASE("[Modulations] Identifiers")
{
    // check that modulations are well defined as either source and target
    // and all targets have their default value defined

    sfz::ModIds::forEachSourceId([](sfz::ModId id)
    {
        REQUIRE(sfz::ModIds::isSource(id));
        REQUIRE(!sfz::ModIds::isTarget(id));
    });

    sfz::ModIds::forEachTargetId([](sfz::ModId id)
    {
        REQUIRE(sfz::ModIds::isTarget(id));
        REQUIRE(!sfz::ModIds::isSource(id));
    });
}

TEST_CASE("[Modulations] Flags")
{
    // check validity of modulation flags

    static auto* checkBasicFlags = +[](int flags)
    {
        REQUIRE(flags != sfz::kModFlagsInvalid);
        REQUIRE((bool(flags & sfz::kModIsPerCycle) +
                 bool(flags & sfz::kModIsPerVoice)) == 1);
    };
    static auto* checkSourceFlags = +[](int flags)
    {
        checkBasicFlags(flags);
        REQUIRE((bool(flags & sfz::kModIsAdditive) +
                 bool(flags & sfz::kModIsMultiplicative)) == 0);
    };
    static auto* checkTargetFlags = +[](int flags)
    {
        checkBasicFlags(flags);
        REQUIRE((bool(flags & sfz::kModIsAdditive) +
                 bool(flags & sfz::kModIsMultiplicative)) == 1);
    };

    sfz::ModIds::forEachSourceId([](sfz::ModId id)
    {
        checkSourceFlags(sfz::ModIds::flags(id));
    });

    sfz::ModIds::forEachTargetId([](sfz::ModId id)
    {
        checkTargetFlags(sfz::ModIds::flags(id));
    });
}

TEST_CASE("[Modulations] Display names")
{
    // check all modulations are implemented in `toString`

    sfz::ModIds::forEachSourceId([](sfz::ModId id)
    {
        REQUIRE(!sfz::ModKey(id).toString().empty());
    });

    sfz::ModIds::forEachTargetId([](sfz::ModId id)
    {
        REQUIRE(!sfz::ModKey(id).toString().empty());
    });
}

TEST_CASE("[Modulations] Connection graph from SFZ")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
<region>
sample=*sine
amplitude_oncc20=59 amplitude_curvecc20=3
pitch_oncc42=71 pitch_smoothcc42=32
pan_oncc36=12.5 pan_stepcc36=0.5
width_oncc425=29
)");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("Controller 20 {curve=3, smooth=0, step=0}" -> "Amplitude {0}")",
        R"("Controller 42 {curve=0, smooth=32, step=0}" -> "Pitch {0}")",
        R"("Controller 36 {curve=0, smooth=0, step=0.04}" -> "Pan {0}")",
        R"("Controller 425 {curve=0, smooth=0, step=0}" -> "Width {0}")",
    }));
}

TEST_CASE("[Modulations] Filter CC connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine
        cutoff=100 fil1_gain_oncc3=5 fil1_gain_stepcc3=0.5
        cutoff2=300 cutoff2_cc2=100 cutoff2_curvecc2=2
        resonance3=-1 resonance3_oncc1=2 resonance3_smoothcc1=10
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("Controller 1 {curve=0, smooth=10, step=0}" -> "FilterResonance {0, N=3}")",
        R"("Controller 2 {curve=2, smooth=0, step=0}" -> "FilterCutoff {0, N=2}")",
        R"("Controller 3 {curve=0, smooth=0, step=0.1}" -> "FilterGain {0, N=1}")",
    }));
}

TEST_CASE("[Modulations] EQ CC connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine
        eq1_gain_oncc2=5 eq1_gain_stepcc2=0.5
        eq2_freq_oncc3=300 eq2_freq_curvecc3=3
        eq3_bw_oncc1=2 eq3_bw_smoothcc1=10
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("Controller 1 {curve=0, smooth=10, step=0}" -> "EqBandwidth {0, N=3}")",
        R"("Controller 2 {curve=0, smooth=0, step=0.1}" -> "EqGain {0, N=1}")",
        R"("Controller 3 {curve=3, smooth=0, step=0}" -> "EqFrequency {0, N=2}")",
    }));
}

TEST_CASE("[Modulations] LFO Filter connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine
        lfo1_freq=0.1 lfo1_cutoff1=1
        lfo2_freq=1 lfo2_cutoff=2
        lfo3_freq=2 lfo3_resonance=3
        lfo4_freq=0.5 lfo4_resonance1=4
        lfo5_freq=0.5 lfo5_resonance2=5
        lfo6_freq=3 lfo6_fil1gain=-1
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("LFO 1 {0}" -> "FilterCutoff {0, N=1}")",
        R"("LFO 2 {0}" -> "FilterCutoff {0, N=1}")",
        R"("LFO 3 {0}" -> "FilterResonance {0, N=1}")",
        R"("LFO 4 {0}" -> "FilterResonance {0, N=1}")",
        R"("LFO 5 {0}" -> "FilterResonance {0, N=2}")",
        R"("LFO 6 {0}" -> "FilterGain {0, N=1}")",
    }));
}

TEST_CASE("[Modulations] EG Filter connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine
        eg1_time1=0.1 eg1_cutoff1=1
        eg2_time1=1 eg2_cutoff=2
        eg3_time1=2 eg3_resonance=3
        eg4_time1=0.5 eg4_resonance1=4
        eg5_time1=0.5 eg5_resonance2=5
        eg6_time1=3 eg6_fil1gain=-1
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("EG 1 {0}" -> "FilterCutoff {0, N=1}")",
        R"("EG 2 {0}" -> "FilterCutoff {0, N=1}")",
        R"("EG 3 {0}" -> "FilterResonance {0, N=1}")",
        R"("EG 4 {0}" -> "FilterResonance {0, N=1}")",
        R"("EG 5 {0}" -> "FilterResonance {0, N=2}")",
        R"("EG 6 {0}" -> "FilterGain {0, N=1}")",
    }));
}

TEST_CASE("[Modulations] LFO EQ connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine
        lfo1_freq=0.1 lfo1_eq1bw=1
        lfo2_freq=1 lfo2_eq2freq=2
        lfo3_freq=2 lfo3_eq3gain=3
        lfo4_freq=0.5 lfo4_eq3bw=4
        lfo5_freq=0.5 lfo5_eq2gain=5
        lfo6_freq=3 lfo6_eq1freq=-1
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("LFO 1 {0}" -> "EqBandwidth {0, N=1}")",
        R"("LFO 2 {0}" -> "EqFrequency {0, N=2}")",
        R"("LFO 3 {0}" -> "EqGain {0, N=3}")",
        R"("LFO 4 {0}" -> "EqBandwidth {0, N=3}")",
        R"("LFO 5 {0}" -> "EqGain {0, N=2}")",
        R"("LFO 6 {0}" -> "EqFrequency {0, N=1}")",
    }));
}

TEST_CASE("[Modulations] EG EQ connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine
        eg1_freq=0.1 eg1_eq1bw=1
        eg2_freq=1 eg2_eq2freq=2
        eg3_freq=2 eg3_eq3gain=3
        eg4_freq=0.5 eg4_eq3bw=4
        eg5_freq=0.5 eg5_eq2gain=5
        eg6_freq=3 eg6_eq1freq=-1
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("EG 1 {0}" -> "EqBandwidth {0, N=1}")",
        R"("EG 2 {0}" -> "EqFrequency {0, N=2}")",
        R"("EG 3 {0}" -> "EqGain {0, N=3}")",
        R"("EG 4 {0}" -> "EqBandwidth {0, N=3}")",
        R"("EG 5 {0}" -> "EqGain {0, N=2}")",
        R"("EG 6 {0}" -> "EqFrequency {0, N=1}")",
    }));
}


TEST_CASE("[Modulations] FlexEG Ampeg target")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg1_time1=0  eg1_level1=1
        eg1_time2=1  eg1_level2=0
        eg1_time3=1  eg1_level3=.5 eg1_sustain=3
        eg1_time4=1  eg1_level4=1
        eg1_ampeg=1
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createModulationDotGraph({
        R"("Controller 10 {curve=1, smooth=10, step=0}" -> "Pan {0}")",
        R"("Controller 11 {curve=4, smooth=10, step=0}" -> "Amplitude {0}")",
        R"("Controller 7 {curve=4, smooth=10, step=0}" -> "Amplitude {0}")",
        R"("EG 1 {0}" -> "MasterAmplitude {0}")",
    }));
}

TEST_CASE("[Modulations] FlexEG Ampeg target with 2 FlexEGs")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg1_time1=0  eg1_level1=1
        eg1_time2=1  eg1_level2=0
        eg1_time3=1  eg1_level3=.5 eg1_sustain=3
        eg1_time4=1  eg1_level4=1
        eg2_time1=0  eg2_level1=1
        eg2_time2=1  eg2_level2=0
        eg2_time3=1  eg2_level3=.5 eg1_sustain=3
        eg2_ampeg=1
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createModulationDotGraph({
        R"("Controller 10 {curve=1, smooth=10, step=0}" -> "Pan {0}")",
        R"("Controller 11 {curve=4, smooth=10, step=0}" -> "Amplitude {0}")",
        R"("Controller 7 {curve=4, smooth=10, step=0}" -> "Amplitude {0}")",
        R"("EG 2 {0}" -> "MasterAmplitude {0}")",
    }));
}


TEST_CASE("[Modulations] FlexEG Ampeg target with multiple EGs targeting ampeg")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg1_time1=0  eg1_level1=1
        eg1_time2=1  eg1_level2=0
        eg1_time3=1  eg1_level3=.5 eg1_sustain=3
        eg1_time4=1  eg1_level4=1
        eg1_ampeg=1
        eg2_time1=0  eg2_level1=1
        eg2_time2=1  eg2_level2=0
        eg2_time3=1  eg2_level3=.5 eg1_sustain=3
        eg2_ampeg=1
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createModulationDotGraph({
        R"("Controller 10 {curve=1, smooth=10, step=0}" -> "Pan {0}")",
        R"("Controller 11 {curve=4, smooth=10, step=0}" -> "Amplitude {0}")",
        R"("Controller 7 {curve=4, smooth=10, step=0}" -> "Amplitude {0}")",
        R"("EG 1 {0}" -> "MasterAmplitude {0}")",
    }));
}

TEST_CASE("[Modulations] Override the default volume controller")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine tune_oncc7=1200
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createModulationDotGraph({
        R"("AmplitudeEG {0}" -> "MasterAmplitude {0}")",
        R"("Controller 10 {curve=1, smooth=10, step=0}" -> "Pan {0}")",
        R"("Controller 11 {curve=4, smooth=10, step=0}" -> "Amplitude {0}")",
        R"("Controller 7 {curve=0, smooth=0, step=0}" -> "Pitch {0}")",
    }));
}

TEST_CASE("[Modulations] Override the default pan controller")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine on_locc10=127 on_hicc10=127
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createModulationDotGraph({
        R"("AmplitudeEG {0}" -> "MasterAmplitude {0}")",
        R"("Controller 11 {curve=4, smooth=10, step=0}" -> "Amplitude {0}")",
        R"("Controller 7 {curve=4, smooth=10, step=0}" -> "Amplitude {0}")",
    }));
}

TEST_CASE("[Modulations] Aftertouch connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine cutoff_chanaft=1000
        <region> sample=*sine cutoff2_chanaft=1000
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("ChannelAftertouch" -> "FilterCutoff {0, N=1}")",
        R"("ChannelAftertouch" -> "FilterCutoff {1, N=2}")",
    }, 2));
}

TEST_CASE("[Modulations] LFO v1 connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine amplfo_freq=1.0
        <region> sample=*sine pitchlfo_freq=1.0
        <region> sample=*sine fillfo_freq=1.0
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("AmplitudeLFO {0}" -> "Volume {0}")",
        R"("PitchLFO {1}" -> "Pitch {1}")",
        R"("FilterLFO {2}" -> "FilterCutoff {2, N=1}")",
    }, 3));
}

TEST_CASE("[Modulations] LFO v1 CC connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine amplfo_depth_oncc1=10
        <region> sample=*sine pitchlfo_depth_oncc2=1200
        <region> sample=*sine fillfo_depth_oncc3=-3600
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("Controller 1 {curve=0, smooth=0, step=0}" -> "AmplitudeLFODepth {0}")",
        R"("Controller 2 {curve=0, smooth=0, step=0}" -> "PitchLFODepth {1}")",
        R"("Controller 3 {curve=0, smooth=0, step=0}" -> "FilterLFODepth {2}")",
        R"("AmplitudeLFO {0}" -> "Volume {0}")",
        R"("PitchLFO {1}" -> "Pitch {1}")",
        R"("FilterLFO {2}" -> "FilterCutoff {2, N=1}")",
    }, 3));
}

TEST_CASE("[Modulations] LFO v1 CC frequency connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine amplfo_freq_oncc1=10
        <region> sample=*sine pitchlfo_freq_cc2=1200
        <region> sample=*sine fillfo_freqcc3=-3600
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("Controller 1 {curve=0, smooth=0, step=0}" -> "AmplitudeLFOFrequency {0}")",
        R"("Controller 2 {curve=0, smooth=0, step=0}" -> "PitchLFOFrequency {1}")",
        R"("Controller 3 {curve=0, smooth=0, step=0}" -> "FilterLFOFrequency {2}")",
        R"("AmplitudeLFO {0}" -> "Volume {0}")",
        R"("PitchLFO {1}" -> "Pitch {1}")",
        R"("FilterLFO {2}" -> "FilterCutoff {2, N=1}")",
    }, 3));
}

TEST_CASE("[Modulations] LFO v1 aftertouch connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine amplfo_depthchanaft=10
        <region> sample=*sine pitchlfo_depthchanaft=1200
        <region> sample=*sine fillfo_depthchanaft=-3600
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("ChannelAftertouch" -> "AmplitudeLFODepth {0}")",
        R"("ChannelAftertouch" -> "PitchLFODepth {1}")",
        R"("ChannelAftertouch" -> "FilterLFODepth {2}")",
        R"("AmplitudeLFO {0}" -> "Volume {0}")",
        R"("PitchLFO {1}" -> "Pitch {1}")",
        R"("FilterLFO {2}" -> "FilterCutoff {2, N=1}")",
    }, 3));
}

TEST_CASE("[Modulations] LFO v1 aftertouch frequency connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine amplfo_freqchanaft=10
        <region> sample=*sine pitchlfo_freqchanaft=1200
        <region> sample=*sine fillfo_freqchanaft=-3600
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("ChannelAftertouch" -> "AmplitudeLFOFrequency {0}")",
        R"("ChannelAftertouch" -> "PitchLFOFrequency {1}")",
        R"("ChannelAftertouch" -> "FilterLFOFrequency {2}")",
        R"("AmplitudeLFO {0}" -> "Volume {0}")",
        R"("PitchLFO {1}" -> "Pitch {1}")",
        R"("FilterLFO {2}" -> "FilterCutoff {2, N=1}")",
    }, 3));
}

TEST_CASE("[Modulations] LFO v1 poly aftertouch connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine amplfo_depthpolyaft=10
        <region> sample=*sine pitchlfo_depthpolyaft=1200
        <region> sample=*sine fillfo_depthpolyaft=-3600
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("PolyAftertouch" -> "AmplitudeLFODepth {0}")",
        R"("PolyAftertouch" -> "PitchLFODepth {1}")",
        R"("PolyAftertouch" -> "FilterLFODepth {2}")",
        R"("AmplitudeLFO {0}" -> "Volume {0}")",
        R"("PitchLFO {1}" -> "Pitch {1}")",
        R"("FilterLFO {2}" -> "FilterCutoff {2, N=1}")",
    }, 3));
}

TEST_CASE("[Modulations] LFO v1 poly aftertouch frequency connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine amplfo_freqpolyaft=10
        <region> sample=*sine pitchlfo_freqpolyaft=1200
        <region> sample=*sine fillfo_freqpolyaft=-3600
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("PolyAftertouch" -> "AmplitudeLFOFrequency {0}")",
        R"("PolyAftertouch" -> "PitchLFOFrequency {1}")",
        R"("PolyAftertouch" -> "FilterLFOFrequency {2}")",
        R"("AmplitudeLFO {0}" -> "Volume {0}")",
        R"("PitchLFO {1}" -> "Pitch {1}")",
        R"("FilterLFO {2}" -> "FilterCutoff {2, N=1}")",
    }, 3));
}

TEST_CASE("[Modulations] EG v1 CC connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine pitcheg_depth_oncc2=1200
        <region> sample=*sine fileg_depth_oncc3=-3600
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("Controller 2 {curve=0, smooth=0, step=0}" -> "PitchEGDepth {0}")",
        R"("Controller 3 {curve=0, smooth=0, step=0}" -> "FilterEGDepth {1}")",
        R"("PitchEG {0}" -> "Pitch {0}")",
        R"("FilterEG {1}" -> "FilterCutoff {1, N=1}")",
    }, 2));
}

TEST_CASE("[Modulations] LFO CC connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine
            pitch_oncc128=1200
            pitch_oncc129=1200
            pitch_oncc131=1200
            pitch_oncc132=1200
            pitch_oncc133=1200
            pitch_oncc134=1200
            pitch_oncc135=1200
            pitch_oncc136=1200
            pitch_oncc137=1200
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("Controller 128 {curve=0, smooth=0, step=0}" -> "Pitch {0}")",
        R"("Controller 129 {curve=0, smooth=0, step=0}" -> "Pitch {0}")",
        R"("PerVoiceController 131 {curve=0, smooth=0, step=0, region=0}" -> "Pitch {0}")",
        R"("PerVoiceController 132 {curve=0, smooth=0, step=0, region=0}" -> "Pitch {0}")",
        R"("PerVoiceController 133 {curve=0, smooth=0, step=0, region=0}" -> "Pitch {0}")",
        R"("PerVoiceController 134 {curve=0, smooth=0, step=0, region=0}" -> "Pitch {0}")",
        R"("PerVoiceController 135 {curve=0, smooth=0, step=0, region=0}" -> "Pitch {0}")",
        R"("PerVoiceController 136 {curve=0, smooth=0, step=0, region=0}" -> "Pitch {0}")",
        R"("PerVoiceController 137 {curve=0, smooth=0, step=0, region=0}" -> "Pitch {0}")",
    }, 1));
}

TEST_CASE("[Modulations] Extended CCs connections")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
        <region> sample=*sine
        lfo1_freq=2 lfo1_freq_cc1=0.1 lfo1_volume=0.5
        lfo2_freq=0.1 lfo2_freq_cc1=2 lfo2_freq_smoothcc1=10 lfo2_freq_stepcc1=0.2 lfo2_freq_curvecc1=1 lfo2_pitch=1200
        lfo3_freq=0.1 lfo3_phase_cc1=2 lfo3_phase_smoothcc1=10 lfo3_phase_stepcc1=0.2 lfo3_phase_curvecc1=1 lfo3_amplitude=50
    )");

    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == createDefaultGraph({
        R"("LFO 1 {0}" -> "Volume {0}")",
        R"("LFO 2 {0}" -> "Pitch {0}")",
        R"("LFO 3 {0}" -> "Amplitude {0}")",
        R"("Controller 1 {curve=0, smooth=0, step=0}" -> "LFOFrequency {0, N=1}")",
        R"("Controller 1 {curve=1, smooth=10, step=0.1}" -> "LFOFrequency {0, N=2}")",
        R"("Controller 1 {curve=1, smooth=10, step=0.1}" -> "LFOPhase {0, N=3}")",
    }, 1));
}
