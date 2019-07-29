cmake_minimum_required(VERSION 3.14)
# You need to generate the AppConfig.h and the JuceHeader.h through the Projucer; the rest is more or less here 

project(sfizz VERSION 1.0.0 LANGUAGES CXX)

# Set the highest possible standard
set(CMAKE_CXX_STANDARD 17)

# Disable Abseil tests
set(BUILD_TESTING OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT ANDROID)
    add_compile_options(-stdlib=libc++)
    # Presumably need the above for linking too, maybe other options missing as well
    add_link_options(-stdlib=libc++)   # New command on CMake master, not in 3.12 release
endif()

add_subdirectory(external/abseil-cpp)
add_subdirectory(external/spdlog)
add_subdirectory(external/Catch2)
add_subdirectory(external/cxxopts)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(COMMON_SOURCES
    sources/Opcode.cpp
    sources/Synth.cpp
    sources/FilePool.cpp
    sources/Region.cpp
    sources/Parser.cpp
)
    
set(TEST_SOURCES
    sources/Opcode.cpp
    sources/Synth.cpp
    sources/Region.cpp
    sources/Parser.cpp
    tests/Regex.cpp
    tests/Helpers.cpp
    tests/Region.cpp
    tests/Range.cpp
    tests/Opcode.cpp
    tests/Buffer.cpp
    tests/AudioBuffer.cpp
    tests/Files.cpp
    tests/Main.cpp
)


###############################
add_executable(sfzprint sources/ParserMain.cpp ${COMMON_SOURCES})
# Per OS properties
if(UNIX)
    target_link_libraries(sfzprint stdc++fs)
endif(UNIX)
target_link_libraries(sfzprint absl::strings cxxopts)


###############################
# Basic command line program
add_executable(sfizz sources/Main.cpp ${COMMON_SOURCES})
# Per OS properties
if(UNIX)
    target_link_libraries(sfizz stdc++fs)
endif(UNIX)
target_link_libraries(sfizz absl::strings cxxopts)

###############################
# Test application
add_executable(sfizz_tests ${TEST_SOURCES} ${COMMON_SOURCES})
# Per OS properties
if(UNIX)
target_link_libraries(sfizz_tests stdc++fs)
endif(UNIX)
target_link_libraries(sfizz_tests Catch2::Catch2 absl::strings)
target_include_directories(sfizz_tests SYSTEM PRIVATE sources)
file(COPY "tests" DESTINATION ${CMAKE_BINARY_DIR})