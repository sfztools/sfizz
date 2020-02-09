// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "ScopedFTZ.h"
#include <benchmark/benchmark.h>

/**
   Override the benchmark entry point to disable the denormals on entry.
 */
#undef BENCHMARK_MAIN
#define BENCHMARK_MAIN()                                            \
    int main(int argc, char** argv) {                               \
        ::benchmark::Initialize(&argc, argv);                       \
        if (::benchmark::ReportUnrecognizedArguments(argc, argv))   \
            return 1;                                               \
        ScopedFTZ ftz;                                              \
        ::benchmark::RunSpecifiedBenchmarks();                      \
    }                                                               \
    int main(int, char**)
