// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Curve.h"
#include "sfizz/Opcode.h"
#include "catch2/catch.hpp"
#include <algorithm>
using namespace Catch::literals;
using namespace sfz::literals;

TEST_CASE("[Curve] Bipolar 0 to 1")
{
    auto curve = sfz::Curve::buildPredefinedCurve(0);
    REQUIRE( curve.evalCC7(0) == 0.0f );
    REQUIRE( curve.evalCC7(64) == 0.5f );
    REQUIRE( curve.evalCC7(127) == 1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(2_norm).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == Approx(63_norm).margin(1e-3) );
    REQUIRE( curve.evalCC7(85) == Approx(85_norm).margin(1e-3) );
    REQUIRE( curve.evalNormalized(0.0f) == 0.0f );
    REQUIRE( curve.evalNormalized(1.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(0.297).margin(1e-3) );
}

TEST_CASE("[Curve] Bipolar -1 to 1")
{
    auto curve = sfz::Curve::buildPredefinedCurve(1);
    REQUIRE( curve.evalCC7(0) == -1.0f );
    REQUIRE( curve.evalCC7(64) == 0.0f );
    REQUIRE( curve.evalCC7(127) == 1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(-1.0f + 2 * 2_norm).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == Approx(-1.0f + 2 * 63_norm).margin(1e-3) );
    REQUIRE( curve.evalCC7(85) == Approx(-1.0f + 2 * 85_norm).margin(1e-3) );
    REQUIRE( curve.evalNormalized(0.0f) == -1.0f );
    REQUIRE( curve.evalNormalized(1.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(-0.405).margin(1e-3) );
}

TEST_CASE("[Curve] Bipolar 1 to 0")
{
    auto curve = sfz::Curve::buildPredefinedCurve(2);
    REQUIRE( curve.evalCC7(0) == 1.0f );
    REQUIRE( curve.evalCC7(64) == 0.5f );
    REQUIRE( curve.evalCC7(127) == 0.0f );
    REQUIRE( curve.evalCC7(2) == Approx(1.0f - 2_norm).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == Approx(1.0f - 63_norm).margin(1e-3) );
    REQUIRE( curve.evalCC7(85) == Approx(1.0f - 85_norm).margin(1e-3) );
    REQUIRE( curve.evalNormalized(0.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(1.0f) == 0.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(0.703).margin(1e-3) );
}

TEST_CASE("[Curve] Bipolar 1 to -1")
{
    auto curve = sfz::Curve::buildPredefinedCurve(3);
    REQUIRE( curve.evalCC7(0) == 1.0f );
    REQUIRE( curve.evalCC7(64) == 0.0f );
    REQUIRE( curve.evalCC7(127) == -1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(1.0f - 2 * 2_norm).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == Approx(1.0f - 2 * 63_norm).margin(1e-3) );
    REQUIRE( curve.evalCC7(85) == Approx(1.0f - 2 * 85_norm).margin(1e-3) );
    REQUIRE( curve.evalNormalized(0.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(1.0f) == -1.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(0.405).margin(1e-3) );
}

TEST_CASE("[Curve] x**2")
{
    auto curve = sfz::Curve::buildPredefinedCurve(4);
    REQUIRE( curve.evalCC7(0) == 0.0f );
    REQUIRE( curve.evalCC7(64) == Approx(0.25).margin(1e-2) );
    REQUIRE( curve.evalCC7(127) == 1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(2_norm * 2_norm).margin(1e-2) );
    REQUIRE( curve.evalCC7(63) == Approx(63_norm * 63_norm).margin(1e-2) );
    REQUIRE( curve.evalCC7(85) == Approx(85_norm * 85_norm).margin(1e-2) );
    REQUIRE( curve.evalNormalized(0.0f) == 0.0f );
    REQUIRE( curve.evalNormalized(1.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(0.09).margin(1e-3) );
}


TEST_CASE("[Curve] sqrt(x)")
{
    auto curve = sfz::Curve::buildPredefinedCurve(5);
    REQUIRE( curve.evalCC7(0) == 0.0f );
    REQUIRE( curve.evalCC7(64) == Approx(sqrtf(2.0f) / 2.0f) );
    REQUIRE( curve.evalCC7(127) == 1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(sqrtf(2_norm)).margin(1e-2) );
    REQUIRE( curve.evalCC7(63) == Approx(sqrtf(63_norm)).margin(1e-2) );
    REQUIRE( curve.evalCC7(85) == Approx(sqrtf(85_norm)).margin(1e-2) );
    REQUIRE( curve.evalNormalized(0.0f) == 0.0f );
    REQUIRE( curve.evalNormalized(1.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(0.54).margin(1e-2) );
}

TEST_CASE("[Curve] sqrt(1-x)")
{
    auto curve = sfz::Curve::buildPredefinedCurve(6);
    REQUIRE( curve.evalCC7(0) == 1.0f );
    REQUIRE( curve.evalCC7(64) == Approx(sqrtf(2.0f) / 2.0f) );
    REQUIRE( curve.evalCC7(127) == 0.0f );
    REQUIRE( curve.evalCC7(2) == Approx(sqrtf(1.0f - 2_norm)).margin(1e-2) );
    REQUIRE( curve.evalCC7(63) == Approx(sqrtf(1.0f - 63_norm)).margin(1e-2) );
    REQUIRE( curve.evalCC7(85) == Approx(sqrtf(1.0f - 85_norm)).margin(1e-2) );
    REQUIRE( curve.evalNormalized(0.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(1.0f) == 0.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(0.84).margin(1e-2) );
}

TEST_CASE("[Curve] Custom")
{
    auto curve = sfz::Curve::buildCurveFromHeader({
        { "v000", "0" },
        { "v063", "1" },
        { "v127", "0" }
    }, sfz::Curve::Interpolator::Linear);
    REQUIRE( curve.evalCC7(0) == 0.0f );
    REQUIRE( curve.evalCC7(127) == 0.0f );
    REQUIRE( curve.evalCC7(63) == 1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(0.032).margin(1e-3) );
    REQUIRE( curve.evalCC7(70) == Approx(0.891).margin(1e-3) );
}

TEST_CASE("[Curve] Custom 2")
{
    auto curve = sfz::Curve::buildCurveFromHeader({
        { "v063", "1" }
    }, sfz::Curve::Interpolator::Linear);
    REQUIRE( curve.evalCC7(0) == 0.0f );
    REQUIRE( curve.evalCC7(127) == 1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(0.032).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == 1.0f );
    REQUIRE( curve.evalCC7(70) == 1.0f );
}

TEST_CASE("[Curve] Custom 3")
{
    auto curve = sfz::Curve::buildCurveFromHeader({
        { "v063", "1" },
        { "v064", "0.5" }
    }, sfz::Curve::Interpolator::Linear);
    REQUIRE( curve.evalCC7(0) == 0.0f );
    REQUIRE( curve.evalCC7(127) == 1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(0.032).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == 1.0f );
    REQUIRE( curve.evalCC7(64) == 0.5f );
    REQUIRE( curve.evalCC7(70) == Approx(0.548).margin(1e-3) );
}

TEST_CASE("[Curve] Custom 4")
{
    auto curve = sfz::Curve::buildCurveFromHeader({
        { "v063", "1" },
        { "v065", "0.5" }
    }, sfz::Curve::Interpolator::Linear);
    REQUIRE( curve.evalCC7(0) == 0.0f );
    REQUIRE( curve.evalCC7(127) == 1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(0.032).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == 1.0f );
    REQUIRE( curve.evalCC7(64) == Approx(0.75).margin(1e-3) );
    REQUIRE( curve.evalCC7(65) == 0.5f );
    REQUIRE( curve.evalCC7(70) == Approx(0.54).margin(1e-3) );
}

TEST_CASE("[Curve] Custom 5")
{
    auto curve = sfz::Curve::buildCurveFromHeader({
        { "v000", "1" },
        { "v064", "0.9" },
        { "v100", "0.9" },
        { "v127", "0" }
    }, sfz::Curve::Interpolator::Linear);
    REQUIRE( curve.evalCC7(0) == 1.0f );
    REQUIRE( curve.evalCC7(15) == Approx(0.977).margin(1e-3) );
    REQUIRE( curve.evalCC7(64) == 0.9f );
    REQUIRE( curve.evalCC7(90) == 0.9f );
    REQUIRE( curve.evalCC7(100) == 0.9f );
    REQUIRE( curve.evalCC7(110) == Approx(0.567).margin(1e-3) );
    REQUIRE( curve.evalCC7(127) == 0.0f );
}

TEST_CASE("[Curve] Add curves to CurveSet")
{
    sfz::CurveSet curveSet;
    curveSet.addCurve(sfz::Curve::buildPredefinedCurve(0));
    curveSet.addCurve(sfz::Curve::buildPredefinedCurve(2));
    REQUIRE( curveSet.getNumCurves() == 2 );
    REQUIRE( curveSet.getCurve(0).evalCC7(0) == 0.0f );
    REQUIRE( curveSet.getCurve(1).evalCC7(0) == 1.0f );
    // Out of bound curve defaults to linear
    REQUIRE( curveSet.getCurve(2).evalCC7(0) == 0.0f );
    REQUIRE( curveSet.getCurve(2).evalCC7(127) == 1.0f );
    // Change a curve in a position
    curveSet.addCurve(sfz::Curve::buildPredefinedCurve(0), 1);
    REQUIRE(curveSet.getNumCurves() == 2);
    REQUIRE( curveSet.getCurve(1).evalCC7(0) == 0.0f );
    // Can't add an implicit curve after the explicit one
    curveSet.addCurve(sfz::Curve::buildPredefinedCurve(0));
    REQUIRE(curveSet.getNumCurves() == 2);
    curveSet.addCurve(sfz::Curve::buildPredefinedCurve(2), 4);
    REQUIRE(curveSet.getNumCurves() == 5);
    REQUIRE( curveSet.getCurve(2).evalCC7(0) == 0.0f ); // Default "empty" curve
    REQUIRE( curveSet.getCurve(3).evalCC7(0) == 0.0f ); // Default "empty" curve
    REQUIRE( curveSet.getCurve(4).evalCC7(0) == 1.0f );
}

TEST_CASE("[Curve] Add bad indices")
{
    sfz::CurveSet curveSet;
    curveSet.addCurve(sfz::Curve::buildPredefinedCurve(0), -2);
    REQUIRE( curveSet.getNumCurves() == 0 );
    curveSet.addCurve(sfz::Curve::buildPredefinedCurve(0), 256);
    REQUIRE( curveSet.getNumCurves() == 0 );
    curveSet.addCurve(sfz::Curve::buildPredefinedCurve(0), 512);
    REQUIRE( curveSet.getNumCurves() == 0 );
}

TEST_CASE("[Curve] Default CurveSet")
{
    auto curveSet = sfz::CurveSet::createPredefined();
    REQUIRE(curveSet.getNumCurves() == 7);
    REQUIRE( curveSet.getCurve(0).evalNormalized(0.0f) == 0.0f );
    REQUIRE( curveSet.getCurve(0).evalNormalized(1.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(0).evalNormalized(0.3f) == Approx(0.297).margin(1e-3) );

    REQUIRE( curveSet.getCurve(1).evalNormalized(0.0f) == -1.0f );
    REQUIRE( curveSet.getCurve(1).evalNormalized(1.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(1).evalNormalized(0.3f) == Approx(-0.405).margin(1e-3) );

    REQUIRE( curveSet.getCurve(2).evalNormalized(0.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(2).evalNormalized(1.0f) == 0.0f );
    REQUIRE( curveSet.getCurve(2).evalNormalized(0.3f) == Approx(0.703).margin(1e-3) );

    REQUIRE( curveSet.getCurve(3).evalNormalized(0.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(3).evalNormalized(1.0f) == -1.0f );
    REQUIRE( curveSet.getCurve(3).evalNormalized(0.3f) == Approx(0.405).margin(1e-3) );

    REQUIRE( curveSet.getCurve(4).evalNormalized(0.0f) == 0.0f );
    REQUIRE( curveSet.getCurve(4).evalNormalized(1.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(4).evalNormalized(0.3f) == Approx(0.09).margin(1e-3) );

    REQUIRE( curveSet.getCurve(5).evalNormalized(0.0f) == 0.0f );
    REQUIRE( curveSet.getCurve(5).evalNormalized(1.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(5).evalNormalized(0.3f) == Approx(0.54).margin(1e-2) );

    REQUIRE( curveSet.getCurve(6).evalNormalized(0.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(6).evalNormalized(1.0f) == 0.0f );
    REQUIRE( curveSet.getCurve(6).evalNormalized(0.3f) == Approx(0.83).margin(1e-2) );
}
