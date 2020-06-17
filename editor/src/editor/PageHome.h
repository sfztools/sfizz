// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <elements.hpp>
#include <absl/strings/string_view.h>
#include <memory>
#include <functional>

namespace el = cycfi::elements;

class PageHome : public el::proxy_base {
public:
    explicit PageHome(el::view& view);
    ~PageHome();

    const el::element& subject() const override;
    el::element& subject() override;

    void updatePreloadSize(int v);
    void updateVolume(float v);
    void updatePolyphony(int v);
    void updateOversampling(int v);
    void updateSfzFile(cycfi::string_view v);

    std::function<void(int)> on_change_preload_size;
    std::function<void(double)> on_change_volume;
    std::function<void(int)> on_change_polyphony;
    std::function<void(int)> on_change_oversampling;
    std::function<void(absl::string_view)> on_change_sfz_file;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
