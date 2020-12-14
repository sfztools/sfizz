# This CMake build file is part of sfizz

cmake_minimum_required(VERSION 3.5)

project(sfizz_kissfft VERSION "1.3.0" LANGUAGES C)

add_library(sfizz_kissfft STATIC
    kiss_fft.c
    tools/kiss_fftr.c)
add_library(sfizz::kissfft ALIAS sfizz_kissfft)
target_include_directories(sfizz_kissfft
    PUBLIC "."
    PUBLIC "tools")
