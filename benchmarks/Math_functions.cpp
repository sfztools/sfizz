#include <benchmark/benchmark.h>
#include <random>
#include <cmath>
#include <iostream>

/*
Cephes Math Library Release 2.2:  June, 1992
Copyright 1984, 1987, 1989 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/

/* Single precision exponential function.
 * test interval: [-0.5, +0.5]
 * trials: 80000
 * peak relative error: 7.6e-8
 * rms relative error: 2.8e-8
 */

static float MAXNUMF = 3.4028234663852885981170418348451692544e38;
static float MAXLOGF = 88.72283905206835;
static float MINLOGF = -103.278929903431851103; /* log(2^-149) */
static float LOG2EF = 1.44269504088896341;
static float C1 =   0.693359375;
static float C2 =  -2.12194440e-4;

float cephes_expf(float xx) {
    float x, z;
    int n;

    x = xx;


    if( x > MAXLOGF)
        {
        //mtherr( "expf", OVERFLOW );
        return( MAXNUMF );
        }

    if( x < MINLOGF )
        {
        //mtherr( "expf", UNDERFLOW );
        return(0.0);
        }

    /* Express e**x = e**g 2**n
    *   = e**g e**( n loge(2) )
    *   = e**( g + n loge(2) )
    */
    z = floorf( LOG2EF * x + 0.5 ); /* floor() truncates toward -infinity. */

    x -= z * C1;
    x -= z * C2;
    n = z;

    z = x * x;
    /* Theoretical peak relative error in [-0.5, +0.5] is 4.2e-9. */
    z =
    ((((( 1.9875691500E-4f  * x
    + 1.3981999507E-3f) * x
    + 8.3334519073E-3f) * x
    + 4.1665795894E-2f) * x
    + 1.6666665459E-1f) * x
    + 5.0000001201E-1f) * z
    + x
    + 1.0;

    /* multiply by power of 2 */
    x = ldexpf( z, n );

    return( x );
}

static void Dummy(benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::normal_distribution<float> dist { };

    for (auto _ : state)
    {
        auto value = dist(gen);
        benchmark::DoNotOptimize(value);
    }
}

static void StdExp(benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::normal_distribution<float> dist { };
    for (auto _ : state)
    {
        auto value = std::exp(dist(gen));
        benchmark::DoNotOptimize(value);
    }
}

static void CephesExp(benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::normal_distribution<float> dist { };
    for (auto _ : state)
    {
        auto value = cephes_expf(dist(gen));
        benchmark::DoNotOptimize(value);
    }
}


BENCHMARK(Dummy);
BENCHMARK(StdExp);
BENCHMARK(CephesExp);
BENCHMARK_MAIN();