#include "sfizz/SIMDHelpers.h"

#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

#include <ghc/fs_std.hpp>

static bool moveToTestsDirectory(const fs::path& searchedPath);

int main(int argc, char* argv[])
{
    if (!moveToTestsDirectory(fs::u8path("tests/TestFiles"))) {
        std::cerr << "Failed to locate test files\n";
        return 1;
    }
    int result = Catch::Session().run(argc, argv);
    return result;
}

static bool moveToTestsDirectory(const fs::path& searchedPath)
{
    while (!fs::exists(searchedPath)) {
        fs::path cwdPath = fs::current_path();
        fs::path newPath = cwdPath.parent_path();
        if (cwdPath == newPath)
            return false;
        fs::current_path(newPath);
    }
    return true;
}
