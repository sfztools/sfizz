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
    REQUIRE( curve.evalCC7(127) == 1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(0.0157).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == Approx(0.496).margin(1e-3) );
    REQUIRE( curve.evalCC7(85) == Approx(0.669).margin(1e-3) );
    REQUIRE( curve.evalNormalized(0.0f) == 0.0f );
    REQUIRE( curve.evalNormalized(1.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(0.299).margin(1e-3) );
}

TEST_CASE("[Curve] Bipolar -1 to 1")
{
    auto curve = sfz::Curve::buildPredefinedCurve(1);
    REQUIRE( curve.evalCC7(0) == -1.0f );
    REQUIRE( curve.evalCC7(127) == 1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(-0.9685).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == Approx(-0.00787).margin(1e-5) );
    REQUIRE( curve.evalCC7(85) == Approx(0.3386).margin(1e-3) );
    REQUIRE( curve.evalNormalized(0.0f) == -1.0f );
    REQUIRE( curve.evalNormalized(1.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(-0.4).margin(1e-3) );
}

TEST_CASE("[Curve] Bipolar 1 to 0")
{
    auto curve = sfz::Curve::buildPredefinedCurve(2);
    REQUIRE( curve.evalCC7(0) == 1.0f );
    REQUIRE( curve.evalCC7(127) == 0.0f );
    REQUIRE( curve.evalCC7(2) == Approx(0.984).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == Approx(0.504).margin(1e-3) );
    REQUIRE( curve.evalCC7(85) == Approx(0.331).margin(1e-3) );
    REQUIRE( curve.evalNormalized(0.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(1.0f) == 0.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(0.701).margin(1e-3) );
}

TEST_CASE("[Curve] Bipolar 1 to -1")
{
    auto curve = sfz::Curve::buildPredefinedCurve(3);
    REQUIRE( curve.evalCC7(0) == 1.0f );
    REQUIRE( curve.evalCC7(127) == -1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(0.9685).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == Approx(0.00787).margin(1e-5) );
    REQUIRE( curve.evalCC7(85) == Approx(-0.3386).margin(1e-3) );
    REQUIRE( curve.evalNormalized(0.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(1.0f) == -1.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(0.4).margin(1e-3) );
}

TEST_CASE("[Curve] x**2")
{
    auto curve = sfz::Curve::buildPredefinedCurve(4);
    REQUIRE( curve.evalCC7(0) == 0.0f );
    REQUIRE( curve.evalCC7(127) == 1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(0.000248).margin(1e-5) );
    REQUIRE( curve.evalCC7(63) == Approx(0.246).margin(1e-3) );
    REQUIRE( curve.evalCC7(85) == Approx(0.448).margin(1e-3) );
    REQUIRE( curve.evalNormalized(0.0f) == 0.0f );
    REQUIRE( curve.evalNormalized(1.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(0.09).margin(1e-3) );
}


TEST_CASE("[Curve] sqrt(x)")
{
    auto curve = sfz::Curve::buildPredefinedCurve(5);
    REQUIRE( curve.evalCC7(0) == 0.0f );
    REQUIRE( curve.evalCC7(127) == 1.0f );
    REQUIRE( curve.evalCC7(2) == Approx(0.125).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == Approx(0.704).margin(1e-3) );
    REQUIRE( curve.evalCC7(85) == Approx(0.818).margin(1e-3) );
    REQUIRE( curve.evalNormalized(0.0f) == 0.0f );
    REQUIRE( curve.evalNormalized(1.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(0.5477).margin(1e-3) );
}

TEST_CASE("[Curve] sqrt(1-x)")
{
    auto curve = sfz::Curve::buildPredefinedCurve(6);
    REQUIRE( curve.evalCC7(0) == 1.0f );
    REQUIRE( curve.evalCC7(127) == 0.0f );
    REQUIRE( curve.evalCC7(2) == Approx(0.992).margin(1e-3) );
    REQUIRE( curve.evalCC7(63) == Approx(0.710).margin(1e-3) );
    REQUIRE( curve.evalCC7(85) == Approx(0.575).margin(1e-3) );
    REQUIRE( curve.evalNormalized(0.0f) == 1.0f );
    REQUIRE( curve.evalNormalized(1.0f) == 0.0f );
    REQUIRE( curve.evalNormalized(0.3f) == Approx(0.837).margin(1e-3) );
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
    REQUIRE( curveSet.getCurve(0).evalNormalized(0.3f) == Approx(0.299).margin(1e-3) );

    REQUIRE( curveSet.getCurve(1).evalNormalized(0.0f) == -1.0f );
    REQUIRE( curveSet.getCurve(1).evalNormalized(1.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(1).evalNormalized(0.3f) == Approx(-0.4).margin(1e-3) );

    REQUIRE( curveSet.getCurve(2).evalNormalized(0.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(2).evalNormalized(1.0f) == 0.0f );
    REQUIRE( curveSet.getCurve(2).evalNormalized(0.3f) == Approx(0.701).margin(1e-3) );

    REQUIRE( curveSet.getCurve(3).evalNormalized(0.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(3).evalNormalized(1.0f) == -1.0f );
    REQUIRE( curveSet.getCurve(3).evalNormalized(0.3f) == Approx(0.4).margin(1e-3) );

    REQUIRE( curveSet.getCurve(4).evalNormalized(0.0f) == 0.0f );
    REQUIRE( curveSet.getCurve(4).evalNormalized(1.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(4).evalNormalized(0.3f) == Approx(0.09).margin(1e-3) );

    REQUIRE( curveSet.getCurve(5).evalNormalized(0.0f) == 0.0f );
    REQUIRE( curveSet.getCurve(5).evalNormalized(1.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(5).evalNormalized(0.3f) == Approx(0.5477).margin(1e-3) );

    REQUIRE( curveSet.getCurve(6).evalNormalized(0.0f) == 1.0f );
    REQUIRE( curveSet.getCurve(6).evalNormalized(1.0f) == 0.0f );
    REQUIRE( curveSet.getCurve(6).evalNormalized(0.3f) == Approx(0.837).margin(1e-3) );
}

TEST_CASE("[Curve] Build from points")
{
    std::array<float, sfz::Curve::NumValues> curvePoints;
    float val = 0.0f;
    float step = 1 / static_cast<float>(sfz::Curve::NumValues);
    for (auto& x : curvePoints) {
        x = val;
        val += step;
    }

    sfz::Curve curve = sfz::Curve::buildFromPoints(curvePoints.data());
    REQUIRE(curve.evalNormalized(0.0) == curvePoints[0]);
    REQUIRE(curve.evalNormalized(1.0) == curvePoints[sfz::Curve::NumValues - 1]);
    REQUIRE(curve.evalNormalized(63_norm) == curvePoints[63]);
}

