// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "absl/strings/string_view.h"
#include <string>
#include <vector>
#include <functional>

class FileDialog {
public:
    enum class Mode { Open, Save };

    struct Filter {
        std::string name;
        std::vector<std::string> patterns;
    };

    void setMode(Mode mode) { mode_ = mode; }
    void setTitle(absl::string_view title) { title_ = std::string(title); }
    void setPath(absl::string_view path) { path_ = std::string(path); }
    void addFilter(Filter filter) { filters_.push_back(std::move(filter)); }

    bool chooseFile();

    std::function<void(absl::string_view)> onFileChosen;

private:
    Mode mode_ = Mode::Open;
    std::string title_;
    std::string path_;
    std::vector<Filter> filters_;
};
