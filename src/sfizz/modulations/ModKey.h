// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "ModKeyHash.h"
#include "../NumericId.h"
#include <string>

namespace sfz {

struct Region;

enum class ModId : int;

/**
 * @brief Identifier of a single modulation source or target within a SFZ instrument
 */
class ModKey {
public:
    struct Parameters;

    ModKey() = default;
    explicit ModKey(ModId id, NumericId<Region> region = {}, Parameters params = {})
        : id_(id), region_(region), params_(params) {}

    static ModKey createCC(uint16_t cc, uint8_t curve, uint8_t smooth, float value, float step);
    static ModKey createNXYZ(ModId id, NumericId<Region> region, uint8_t N = 0, uint8_t X = 0, uint8_t Y = 0, uint8_t Z = 0);

    explicit operator bool() const noexcept { return id_ != ModId(); }

    const ModId& id() const noexcept { return id_; }
    NumericId<Region> region() const noexcept { return region_; }
    const Parameters& parameters() const noexcept { return params_; }

    bool isSource() const noexcept;
    bool isTarget() const noexcept;
    int flags() const noexcept;
    std::string toString() const;

    struct Parameters {
        Parameters() noexcept;
        Parameters(const Parameters& other) noexcept;
        Parameters& operator=(const Parameters& other) noexcept;

        Parameters(Parameters&&) = delete;
        Parameters &operator=(Parameters&&) = delete;

        bool operator==(const Parameters& other) const noexcept;
        bool operator!=(const Parameters& other) const noexcept;

        union {
            //! Parameters if this key identifies a CC source
            struct { uint16_t cc; uint8_t curve, smooth; float value, step; };
            //! Parameters otherwise, based on the related opcode
            // eg. `N` in `lfoN`, `N, X` in `lfoN_eqX`
            struct { uint8_t N, X, Y, Z; };
        };
    };

public:
    bool operator==(const ModKey &other) const noexcept;
    bool operator!=(const ModKey &other) const noexcept;

private:
    //! Identifier
    ModId id_ {};
    //! Region identifier, only applicable if the modulation is per-voice
    NumericId<Region> region_;
    //! List of values which identify the key uniquely, along with the hash and region
    Parameters params_ {};
};

} // namespace sfz
