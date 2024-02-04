// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ModKeyHash.h"
#include "ModKey.h"
#include "ModId.h"
#include "utility/StringViewHelpers.h"
#include <cstdint>

void sfz::ModKey::calculateHash()
{
#if !defined(NDEBUG)
    static bool once = false;
    if (!once) {
        once = true;
        ModKey m;
        size_t h = m.hash();
        m.calculateHash();
        if (h != m.hash()) {
            printf("ModKey default hash is %llu\n", (uint64_t)m.hash());
            assert(false && "Number of variables is wrong. Needs updating the default hash.");
        }
    }
#endif

    uint64_t k = hashNumber(static_cast<int>(id()));
    k = hashNumber(region_.number(), k);
    const sfz::ModKey::Parameters& p = parameters();

    switch (id()) {
    case sfz::ModId::Controller:
        k = hashNumber(p.cc, k);
        k = hashNumber(p.curve, k);
        k = hashNumber(p.smooth, k);
        k = hashNumber(p.step, k);
        break;
    default:
        k = hashNumber(p.N, k);
        k = hashNumber(p.X, k);
        k = hashNumber(p.Y, k);
        k = hashNumber(p.Z, k);
        break;
    }
    hash_ = size_t(k);
}

size_t std::hash<sfz::ModKey>::operator()(const sfz::ModKey &key) const
{
    return key.hash();
}
