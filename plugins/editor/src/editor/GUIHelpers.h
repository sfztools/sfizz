// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "utility/vstgui_before.h"
#include <vstgui/lib/cframe.h>
#include <vstgui/lib/cvstguitimer.h>
#include "utility/vstgui_after.h"

using namespace VSTGUI;

class SFrameDisabler : public CBaseObject {
public:
    explicit SFrameDisabler(CFrame* frame);
    ~SFrameDisabler();

    void enable();
    void disable();

private:
    class KeyAndMouseHook;

private:
    CFrame* frame_ = nullptr;
    SharedPointer<KeyAndMouseHook> hook_;
    SharedPointer<CVSTGUITimer> delayedEnabler_;
};
