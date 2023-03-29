// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "utility/vstgui_before.h"
#include "vstgui/lib/cviewcontainer.h"
#include "vstgui/vstgui.h"
#include "utility/vstgui_after.h"
#include <string>

#define VSTGUI_MORE_THAN_4_10 ((VSTGUI_VERSION_MAJOR > 4) \
    || (VSTGUI_VERSION_MAJOR == 4 && VSTGUI_VERSION_MINOR > 10))

using namespace VSTGUI;

class SAboutDialog : public CViewContainer, public IControlListener, public IKeyboardHook {

    enum {
        kTagButtonSfztools,
        kTagButtonGithub,
        kTagButtonDiscord,
        kTagButtonOpencollective,
        kTagButtonSfzformat
    };

public:
    explicit SAboutDialog(const CRect& bounds);

    void setPluginFormat(const std::string& pluginFormat);
    void setPluginHost(const std::string& pluginHost);

#if VSTGUI_MORE_THAN_4_10
    void onKeyboardEvent (KeyboardEvent& event, CFrame* frame) override;
#else
    int32_t onKeyDown(const VstKeyCode& code, CFrame* frame) override;
    int32_t onKeyUp(const VstKeyCode& code, CFrame* frame) override;
#endif

protected:
    CMouseEventResult onMouseDown(CPoint& where, const CButtonState& buttons) override;

    // IControlListener
    void valueChanged(CControl* ctl) override;

private:
    void updateSysInfo();

    void buttonHoverEnter(CControl* btn, const char* text);
    void buttonHoverLeave(CControl* btn);

    CTextLabel* lblHover_ = {};
    CTextLabel* lblSysInfoValue_ = {};
    std::string sysInfoTemplate_;
    std::map<std::string, std::string> sysInfoVariables_;
};
