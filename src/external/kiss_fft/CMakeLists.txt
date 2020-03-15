# This CMake build file is part of sfizz

cmake_minimum_required(VERSION 3.5)

project(sfizz-kissfft VERSION "1.3.0" LANGUAGES C)

add_library(sfizz-kissfft STATIC
    kiss_fft.c
    tools/kiss_fftr.c)
target_include_directories(sfizz-kissfft
    PUBLIC "."
    PUBLIC "tools")
