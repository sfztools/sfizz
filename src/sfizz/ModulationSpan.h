// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <absl/types/span.h>
#include <cstddef>

namespace sfz {

/**
 * @brief A modulation span stores the result of a modulation generator.
 *
 * If the result is none (does not modulate), the bool operator returns false.
 *
 * Otherwise, the * operator returns a span.
 * In addition, if the modulation is invariant over the entire time span,
 * the generator can mark the modulation as such, in order to enable more
 * efficient code paths.
 */
class ModulationSpan {
public:
    constexpr ModulationSpan() noexcept = default;

    enum {
        kInvariant = 1 << 0,
    };

    explicit ModulationSpan(absl::Span<const float> span, int flags = 0)
        : ModulationSpan(span.data(), span.size(), flags)
    {
    }
    ModulationSpan(const float* data, size_t size, int flags = 0)
        : data_(data), size_(size), flags_(flags)
    {
    }

    explicit operator bool() const noexcept
    {
        return data_ != nullptr;
    }

    absl::Span<const float> operator*() const noexcept
    {
        return { data_, size_ };
    }

    bool isInvariant() const noexcept
    {
        return (flags_ & kInvariant) != 0;
    }

private:
    const float* data_ = nullptr;
    size_t size_ = 0;
    int flags_ = 0;
};

} // namespace sfz
