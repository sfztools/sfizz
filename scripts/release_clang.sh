#!/bin/sh
script_dir="$(dirname "$0")"
cmake -D CMAKE_C_COMPILER=clang-9 -D CMAKE_CXX_COMPILER=clang++-9 -D CMAKE_BUILD_TYPE=Release -S "$script_dir/.." -B .