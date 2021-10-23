// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "catch2/catch.hpp"
#include <jsl/memory>
#include <vector>
#include <cstdint>

TEST_CASE("[Memory] Aligned unique pointers")
{
    constexpr size_t numAllocations = 128;
    constexpr size_t alignment = 1024;

    static size_t numLiveObjects;
    numLiveObjects = 0;

    struct Object {
        explicit Object(size_t id) : id(id) { ++numLiveObjects; }
        ~Object() { --numLiveObjects; }
        size_t id {};
    };

    std::vector<jsl::aligned_unique_ptr<Object, alignment>> ptrs(numAllocations);

    for (size_t i = 0; i < numAllocations; ++i) {
        ptrs[i] = jsl::make_aligned<Object, alignment>(i);
        REQUIRE(numLiveObjects == i + 1);

        Object *obj = ptrs[i].get();
        REQUIRE(obj->id == i);

        uintptr_t addr = reinterpret_cast<uintptr_t>(obj);
        REQUIRE(addr % alignment == 0);
    }

    ptrs.clear();
    REQUIRE(numLiveObjects == 0);
}
