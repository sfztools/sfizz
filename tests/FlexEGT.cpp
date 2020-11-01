// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz


#include "sfizz/Synth.h"
#include "sfizz/AudioBuffer.h"
#include "sfizz/FlexEnvelope.h"
#include "catch2/catch.hpp"
#include "TestHelpers.h"
#include <array>
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
    REQUIRE(synth.getResources().modMatrix.toDotGraph() == createDefaultGraph({
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
    REQUIRE( synth.getResources().modMatrix.toDotGraph() == createDefaultGraph({}) );
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
    REQUIRE( synth.getResources().modMatrix.toDotGraph() == createDefaultGraph({
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

TEST_CASE("[FlexEG] Zero delay transitions")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg1_time1=0  eg1_level1=1
        eg1_time2=1  eg1_level2=0
        eg1_time3=1  eg1_level3=.5 eg1_sustain=3
        eg1_time4=1  eg1_level4=1
    )");
    sfz::FlexEnvelope envelope;
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->flexEGs.size() == 1);
    envelope.configure(&synth.getRegionView(0)->flexEGs[0]);
    envelope.setSampleRate(10);
    envelope.start(1);

    std::array<float, 2> output;
    envelope.process(absl::MakeSpan(output));
    REQUIRE(output[0] == 0.0f);
    REQUIRE(output[1] == Approx(0.9f).margin(0.01f));
    // Note(jpc): 0.9 is because EG pre-increments the time counter, slope is
    //            1 frame off into the future
}

TEST_CASE("[FlexEG] Early release")
{
    for (int i = 0; i < 3; ++i) {
        sfz::Synth synth;

        synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine
        eg1_ampeg=1
        eg1_time1=1.0  eg1_level1=1.0
        eg1_time2=1.0  eg1_level2=1.0 eg1_sustain=2
        eg1_time3=1.0  eg1_level3=0.0
    )");
        sfz::FlexEnvelope envelope;
        REQUIRE(synth.getNumRegions() == 1);
        REQUIRE(synth.getRegionView(0)->flexEGs.size() == 1);
        envelope.configure(&synth.getRegionView(0)->flexEGs[0]);
        envelope.setSampleRate(100);

        envelope.start(0);
        switch (i) {
        case 0:
            // A normal release: up 1s, sustain 1s, down 1s
            envelope.release(200);
            break;
        case 1:
            // A fast release: up 1s, down 1s
            envelope.release(100);
            break;
        case 2:
            // A faster release: up 0.5s, down 0.5s
            envelope.release(50);
            break;
        }

        std::array<float, 400> output;
        envelope.process(absl::MakeSpan(output));

        // Theoretical output at 0.5s interval
        const std::array<float, 7> ref0 {{ 0.0, 0.5, 1.0, 1.0, 1.0, 0.5, 0.0 }};
        const std::array<float, 5> ref1 {{ 0.0, 0.5, 1.0, 0.5, 0.0 }};
        const std::array<float, 3> ref2 {{ 0.0, 0.5, 0.25 }};

        const float m = 0.015f;
        switch (i) {
        case 0:
            REQUIRE(output[  0] == Approx(ref0[0]).margin(m));
            REQUIRE(output[ 50] == Approx(ref0[1]).margin(m));
            REQUIRE(output[100] == Approx(ref0[2]).margin(m));
            REQUIRE(output[150] == Approx(ref0[3]).margin(m));
            REQUIRE(output[200] == Approx(ref0[4]).margin(m));
            REQUIRE(output[250] == Approx(ref0[5]).margin(m));
            REQUIRE(output[300] == Approx(ref0[6]).margin(m));
            break;
        case 1:
            REQUIRE(output[  0] == Approx(ref1[0]).margin(m));
            REQUIRE(output[ 50] == Approx(ref1[1]).margin(m));
            REQUIRE(output[100] == Approx(ref1[2]).margin(m));
            REQUIRE(output[150] == Approx(ref1[3]).margin(m));
            REQUIRE(output[200] == Approx(ref1[4]).margin(m));
            break;
        case 2:
            REQUIRE(output[  0] == Approx(ref2[0]).margin(m));
            REQUIRE(output[ 50] == Approx(ref2[1]).margin(m));
            REQUIRE(output[100] == Approx(ref2[2]).margin(m));
            break;
        }
    }
}

TEST_CASE("[FlexEG] Free-running flex AmpEG (no sustain)")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*noise
            key=60
            loop_mode=one_shot
            eg1_ampeg=1
            eg1_time1=0    eg1_level1=1
            eg1_time2=0.03 eg1_level2=0.6
            eg1_time3=0.06 eg1_level3=0.3
            eg1_time4=0.12 eg1_level4=0.1
            eg1_time5=0.3  eg1_level5=0
        <region> sample=*noise
            key=62
            loop_mode=one_shot
            eg1_ampeg=1
            eg1_time1=0    eg1_level1=1
            eg1_time2=0.03 eg1_level2=0.6
            eg1_time3=0.06 eg1_level3=0.3
            eg1_time4=0.12 eg1_level4=0.1
            eg1_time5=0.3  eg1_level5=0 eg1_sustain=5
        <region> sample=*noise
            key=64
            eg1_ampeg=1
            eg1_time1=0    eg1_level1=1
            eg1_time2=0.03 eg1_level2=0.6
            eg1_time3=0.06 eg1_level3=0.3
            eg1_time4=0.12 eg1_level4=0.1
            eg1_time5=0.3  eg1_level5=0 eg1_sustain=5
    )");
    synth.noteOn(0, 60, 0);
    sfz::AudioBuffer<float> buffer { 2, 256 };
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    for (unsigned i = 0; i < 100; ++i)
        synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );

    synth.noteOn(0, 62, 0);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    for (unsigned i = 0; i < 100; ++i)
        synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );

    synth.noteOn(0, 64, 0);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    for (unsigned i = 0; i < 100; ++i)
        synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    synth.noteOff(0, 64, 0); // the release stage is 0 duration
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
}
