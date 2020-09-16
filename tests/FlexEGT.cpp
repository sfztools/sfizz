// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz


#include "sfizz/Synth.h"
#include "sfizz/FlexEnvelope.h"
#include "catch2/catch.hpp"
#include "TestHelpers.h"
using namespace Catch::literals;
using namespace sfz::literals;

TEST_CASE("[FlexEG] Values")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg1_amplitude=1
        eg1_time1=.1  eg1_level1=.25
        eg1_time2=.2  eg1_level2=1
        eg1_time3=.2  eg1_level3=.5 eg1_sustain=3
        eg1_time4=.4  eg1_level4=1
    )");
    REQUIRE(synth.getNumRegions() == 1);
    const auto* region = synth.getRegionView(0);
    REQUIRE( region->flexEGs.size() == 1 );
    const auto& egDescription = region->flexEGs[0];
    REQUIRE( egDescription.points.size() == 5 );
    REQUIRE( egDescription.points[0].time == 0.0_a );
    REQUIRE( egDescription.points[0].level == 0.0_a );
    REQUIRE( egDescription.points[1].time == .1_a );
    REQUIRE( egDescription.points[1].level == .25_a );
    REQUIRE( egDescription.points[2].time == .2_a );
    REQUIRE( egDescription.points[2].level == 1.0_a );
    REQUIRE( egDescription.points[3].time == .2_a );
    REQUIRE( egDescription.points[3].level == .5_a );
    REQUIRE( egDescription.points[4].time == .4_a );
    REQUIRE( egDescription.points[4].level == 1.0_a );
    REQUIRE( egDescription.sustain == 3 );
    REQUIRE(synth.getResources().modMatrix.toDotGraph() == createReferenceGraph({
        R"("EG 1 {0}" -> "Amplitude {0}")",
    }));
}

TEST_CASE("[FlexEG] Default values")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg3_time2=.1  eg3_level2=.25
    )");
    REQUIRE(synth.getNumRegions() == 1);
    const auto* region = synth.getRegionView(0);
    REQUIRE( region->flexEGs.size() == 3 );
    REQUIRE( region->flexEGs[0].points.size() == 0 );
    REQUIRE( region->flexEGs[1].points.size() == 0 );
    const auto& egDescription = region->flexEGs[2];
    REQUIRE( egDescription.points.size() == 3 );
    REQUIRE( egDescription.points[0].time == 0.0_a );
    REQUIRE( egDescription.points[0].level == 0.0_a );
    REQUIRE( egDescription.points[1].time == 0.0_a );
    REQUIRE( egDescription.points[1].level == 0.0_a );
    REQUIRE( egDescription.points[2].time == .1_a );
    REQUIRE( egDescription.points[2].level == .25_a );
    REQUIRE( synth.getResources().modMatrix.toDotGraph() == createReferenceGraph({}) );
}

TEST_CASE("[FlexEG] Connections")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine eg1_amplitude=1 eg1_time1=.1  eg1_level1=.25
        <region> sample=*sine eg1_pan=1 eg1_time1=.1  eg1_level1=.25
        <region> sample=*sine eg1_width=1 eg1_time1=.1  eg1_level1=.25
        <region> sample=*sine eg1_position=1 eg1_time1=.1  eg1_level1=.25
        <region> sample=*sine eg1_pitch=1 eg1_time1=.1  eg1_level1=.25
        <region> sample=*sine eg1_volume=1 eg1_time1=.1  eg1_level1=.25
    )");
    REQUIRE(synth.getNumRegions() == 6);
    REQUIRE( synth.getRegionView(0)->flexEGs.size() == 1 );
    REQUIRE( synth.getRegionView(0)->flexEGs[0].points.size() == 2 );
    REQUIRE( synth.getResources().modMatrix.toDotGraph() == createReferenceGraph({
        R"("EG 1 {0}" -> "Amplitude {0}")",
        R"("EG 1 {1}" -> "Pan {1}")",
        R"("EG 1 {2}" -> "Width {2}")",
        R"("EG 1 {3}" -> "Position {3}")",
        R"("EG 1 {4}" -> "Pitch {4}")",
        R"("EG 1 {5}" -> "Volume {5}")",
    }, 6));
}

TEST_CASE("[FlexEG] Coarse numerical envelope test (No release)")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg1_time1=.5  eg1_level1=.25
        eg1_time2=0.5  eg1_level2=1
        eg1_sustain=2
    )");
    sfz::FlexEnvelope envelope;
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE( synth.getRegionView(0)->flexEGs.size() == 1 );
    envelope.configure(&synth.getRegionView(0)->flexEGs[0]);
    std::vector<float> output;
    envelope.setSampleRate(10);
    output.resize(16);
    envelope.start(1);
    envelope.process(absl::MakeSpan(output));
    REQUIRE( output[0] == 0.0_a ); // Trigger delay
    REQUIRE( output[5] == 0.25_a ); // 0.25 at time == 0.5s (5 samples at samplerate 10 + trigger delay)
    REQUIRE( output[10] == 1.0_a ); // 1 at time == 1s (5 samples at samplerate 10 + trigger delay)
    REQUIRE( output[15] == 1.0_a ); // sustaining
}

TEST_CASE("[FlexEG] Detailed numerical envelope test")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg1_time1=.5  eg1_level1=.25
        eg1_time2=0.5  eg1_level2=1
        eg1_sustain=2
    )");
    sfz::FlexEnvelope envelope;
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE( synth.getRegionView(0)->flexEGs.size() == 1 );
    envelope.configure(&synth.getRegionView(0)->flexEGs[0]);
    std::vector<float> output;
    std::vector<float> expected { 0.0f, 0.05f, 0.1f, 0.15f, 0.2f, 0.25f, 0.4f, 0.55f, 0.7f, 0.85f, 1.0f, 1.0f, 1.0f };
    output.resize(expected.size());
    envelope.setSampleRate(10);
    envelope.start(1);
    envelope.process(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected) );
}

TEST_CASE("[FlexEG] Coarse numerical envelope test (with release)")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg1_time1=.5  eg1_level1=.25
        eg1_time2=0.5  eg1_level2=1
        eg1_sustain=2
    )");
    sfz::FlexEnvelope envelope;
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE( synth.getRegionView(0)->flexEGs.size() == 1 );
    envelope.configure(&synth.getRegionView(0)->flexEGs[0]);
    std::vector<float> output;
    envelope.setSampleRate(10);
    output.resize(32);
    envelope.start(1);
    envelope.release(15);
    envelope.process(absl::MakeSpan(output));
    REQUIRE( output[0] == 0.0_a ); // Trigger delay
    REQUIRE( output[5] == 0.25_a ); // 0.25 at time == 0.5s (5 samples at samplerate 10 + trigger delay)
    REQUIRE( output[10] == 1.0_a ); // 1 at time == 1s (5 samples at samplerate 10 + trigger delay)
    REQUIRE( output[15] == 1.0_a ); // sustaining
    REQUIRE( output[16] == 0.0_a ); // released
    REQUIRE( output[31] == 0.0_a ); // released
}

TEST_CASE("[FlexEG] Detailed numerical envelope test (with release and release ramp)")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg1_time1=.5  eg1_level1=.25
        eg1_time2=0.5  eg1_level2=1
        eg1_time3=0.5  eg1_level3=0
        eg1_sustain=2
    )");
    sfz::FlexEnvelope envelope;
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE( synth.getRegionView(0)->flexEGs.size() == 1 );
    envelope.configure(&synth.getRegionView(0)->flexEGs[0]);
    std::vector<float> output;
    std::vector<float> expected {
        0.0f,
        0.05f, 0.1f, 0.15f, 0.2f, 0.25f,
        0.4f, 0.55f, 0.7f, 0.85f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        0.8f, 0.6f, 0.4f, 0.2f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };
    output.resize(expected.size());
    envelope.setSampleRate(10);
    envelope.start(1);
    envelope.release(15);
    envelope.process(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected) );
}

TEST_CASE("[FlexEG] Coarse numerical envelope test (with shapes)")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg1_time1=.5  eg1_level1=.25 eg1_shape1=2
        eg1_time2=0.5  eg1_level2=1 eg1_shape2=0.5
        eg1_sustain=2
        eg1_time3=0.5  eg1_level3=0 eg1_shape3=4
    )");
    sfz::FlexEnvelope envelope;
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE( synth.getRegionView(0)->flexEGs.size() == 1 );
    envelope.configure(&synth.getRegionView(0)->flexEGs[0]);
    std::vector<float> output;
    envelope.setSampleRate(10);
    output.resize(32);
    envelope.start(1);
    envelope.release(15);
    envelope.process(absl::MakeSpan(output));
    REQUIRE( output[0] == 0.0_a ); // Trigger delay
    REQUIRE( output[5] == 0.25_a ); // 0.25 at time == 0.5s (5 samples at samplerate 10 + trigger delay)
    REQUIRE( output[10] == 1.0_a ); // 1 at time == 1s (5 samples at samplerate 10 + trigger delay)
    REQUIRE( output[15] == 1.0_a ); // sustaining
    REQUIRE( output[31] == 0.0_a ); // released
}

TEST_CASE("[FlexEG] Detailed numerical envelope test (with shapes)")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg1_time1=.5  eg1_level1=.25 eg1_shape1=2
        eg1_time2=0.5  eg1_level2=1 eg1_shape2=0.5
        eg1_time3=0.5  eg1_level3=0 eg1_shape3=4
        eg1_sustain=2
    )");
    sfz::FlexEnvelope envelope;
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE( synth.getRegionView(0)->flexEGs.size() == 1 );
    envelope.configure(&synth.getRegionView(0)->flexEGs[0]);
    std::vector<float> output;
    std::vector<float> expected {
        0.0f,
        0.01f, 0.04f, 0.09f, 0.16f, 0.25f,
        0.58f, 0.72f, 0.83f, 0.92f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        0.99f, 0.97f, 0.87f, 0.59f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };
    output.resize(expected.size());
    envelope.setSampleRate(10);
    envelope.start(1);
    envelope.release(15);
    envelope.process(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected, 0.01f) );
}
