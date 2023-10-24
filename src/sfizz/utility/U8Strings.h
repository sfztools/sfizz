// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include "ghc/fs_std.hpp"
#include <string>

inline std::string from_u8string(const std::string &s) {
    return s;
}

inline std::string from_u8string(std::string &&s) {
    return std::move(s);
}

#if defined(__cpp_lib_char8_t)
inline std::string from_u8string(const std::u8string &s) {
    return std::string(s.begin(), s.end());
}
#endif

inline std::string u8EncodedString(const fs::path& path) {
    return from_u8string(path.u8string());
}
