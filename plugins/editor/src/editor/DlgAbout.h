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

using namespace VSTGUI;

class SAboutDialog : public CViewContainer, public IControlListener {

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
