// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <absl/strings/string_view.h>
#include <absl/types/optional.h>

class SfizzSettings {
public:
    absl::optional<std::string> load(const char* key);
    std::string load_or(const char* key, absl::string_view defaultValue);
    bool store(const char* key, absl::string_view value);
};
