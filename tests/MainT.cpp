#include "sfizz/SIMDHelpers.h"

#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

#include <ghc/fs_std.hpp>

int main(int argc, char* argv[])
{
    if (!fs::exists(SFIZZ_TEST_FILES) || !fs::exists(SFIZZ_TEST_DIR)) {
        std::cerr << "Failed to locate test files\n";
        return 1;
    }
    int result = Catch::Session().run(argc, argv);
    return result;
}
