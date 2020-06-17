// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <elements.hpp>
#include <absl/strings/string_view.h>
#include <functional>
#include <memory>

namespace el = cycfi::elements;

class PageSettings : public el::proxy_base {
public:
    explicit PageSettings(el::view& view_);
    ~PageSettings();

    const el::element& subject() const override;
    el::element& subject() override;

    void updateScalaFile(cycfi::string_view v);
    void updateScalaRootKey(float v);
    void updateTuningFrequency(float v);
    void updateStretchTuning(float v);

    std::function<void(absl::string_view)> on_change_scala_file;
    std::function<void(int)> on_change_scala_root_key;
    std::function<void(double)> on_change_tuning_frequency;
    std::function<void(double)> on_change_stretch_tuning;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
