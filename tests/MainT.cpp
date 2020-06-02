#include "sfizz/SIMDHelpers.h"

#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

int main(int argc, char* argv[])
{
    sfz::SIMDInitializer simdInit;

    int result = Catch::Session().run(argc, argv);
    return result;
}
