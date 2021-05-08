// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <ghc/fs_std.hpp>
#include <vector>

const fs::path& getUserDocumentsDirectory();

#if !defined(_WIN32) && !defined(__APPLE__)
const fs::path& getUserHomeDirectory();
const fs::path& getXdgConfigHome();
struct XdgUserDirsEntry { std::string name; fs::path value; };
std::vector<XdgUserDirsEntry> parseXdgUserDirs(const fs::path& userDirsPath);
#endif
