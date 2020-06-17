// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <elements.hpp>
#include <memory>
class EditorController;

namespace el = cycfi::elements;

class UI {
public:
    UI(el::view& group, EditorController& ctrl);
    ~UI();

    void updatePreloadSize(int v);
    void updateVolume(float v);
    void updatePolyphony(float v);
    void updateOversampling(int v);
    void updateSfzFile(cycfi::string_view v);
    void updateScalaFile(cycfi::string_view v);
    void updateScalaRootKey(float v);
    void updateTuningFrequency(float v);
    void updateStretchTuning(float v);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
