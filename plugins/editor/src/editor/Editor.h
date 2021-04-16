// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <sfizz_message.h>
#include <memory>
class EditorController;

#include "utility/vstgui_before.h"
#include "vstgui/lib/vstguifwd.h"
#include "utility/vstgui_after.h"
using VSTGUI::CFrame;

class Editor {
public:
    static const int viewWidth;
    static const int viewHeight;

    explicit Editor(EditorController& ctrl);
    ~Editor();

    void open(CFrame& frame);
    void close();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
