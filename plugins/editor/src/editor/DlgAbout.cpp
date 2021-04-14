// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "DlgAbout.h"
#include "GUIComponents.h"
#include "GitBuildId.h"
#include "NativeHelpers.h"

#include "utility/vstgui_before.h"
#include "vstgui/vstgui.h"
#include "utility/vstgui_after.h"

#include <absl/strings/str_cat.h>

SAboutDialog::SAboutDialog(const CRect& bounds)
    : CViewContainer(bounds)
{
    SharedPointer<CBitmap> logo = owned(new CBitmap("logo_orange.png"));
    setBackgroundColor(CColor(0x00, 0x00, 0x00, 0xc0));

    CView* aboutView = nullptr;
    {
        auto createaboutView = [](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            CViewContainer* container = new CViewContainer(bounds);
            container->setBackgroundColor(CColor(0x00, 0x00, 0x00, 0x00));
            return container;
        };

        auto createLogo = [&logo](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            CViewContainer* container = new CViewContainer(bounds);
            container->setBackgroundColor(CColor(0x00, 0x00, 0x00, 0x00));
            container->setBackground(logo);
            return container;
        };

        auto createInfoBox = [](const CRect& bounds, int, const char*, CHoriTxtAlign align, int fontsize) {
            CMultiLineTextLabel* lbl = new CMultiLineTextLabel(bounds);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            lbl->setFont(font);
            lbl->setFontColor(CColor(0xff, 0xff, 0xff, 0xff));
            lbl->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setHoriAlign(align);

            const char* version = SFIZZ_VERSION;
            std::string versionBuf;
            if (GitBuildId[0]) {
                versionBuf = absl::StrCat(SFIZZ_VERSION ".", GitBuildId);
                version = versionBuf.c_str();
            }
            lbl->setText(absl::StrCat(
                u8"Version ", version, u8"\n"
                u8"Copyright 2019-2021 by SFZTools Team,\n"
                u8"licensed under BSD 2-clause license."));
            return lbl;
        };

        auto createDescriptionBox = [](const CRect& bounds, int, const char*, CHoriTxtAlign align, int fontsize) {
            CMultiLineTextLabel* lbl = new CMultiLineTextLabel(bounds);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            lbl->setFont(font);
            lbl->setFontColor(CColor(0xff, 0xff, 0xff, 0xff));
            lbl->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setHoriAlign(align);
            lbl->setText(absl::StrCat(
                u8"Maintainers: Paul Ferrand, Jean-Pierre Cimalando\n"
                u8"Contributors: Andrea Zanellato, Alexander Mitchell, Michael Willis,\n"
                u8"Tobiasz \"unfa\" Karoń, Kinwie, Atsushi Eno, Dominique Würtz,\n"
                u8"Even Brenden et al."));
            return lbl;
        };

        auto createHoverBox = [](const CRect& bounds, int, const char* label, CHoriTxtAlign align, int fontsize) {
            CTextLabel* lbl = new CTextLabel(bounds, label);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            lbl->setFontColor(CColor(0xfd, 0x98, 0x00, 0xff));
            lbl->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setHoriAlign(align);
            lbl->setFont(font);
            return lbl;
        };

        auto createSysInfoKeyBox = [](const CRect& bounds, int, const char* label, CHoriTxtAlign align, int fontsize) {
            CMultiLineTextLabel* lbl = new CMultiLineTextLabel(bounds);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize, kBoldFace);
            lbl->setFont(font);
            lbl->setFontColor(CColor(0xfd, 0x98, 0x00, 0xff));
            lbl->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setHoriAlign(align);
            lbl->setText(label);
            return lbl;
        };

        auto createSysInfoValueBox = [](const CRect& bounds, int, const char* label, CHoriTxtAlign align, int fontsize) {
            CMultiLineTextLabel* lbl = new CMultiLineTextLabel(bounds);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            lbl->setFont(font);
            lbl->setFontColor(CColor(0xff, 0xff, 0xff, 0xff));
            lbl->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setHoriAlign(align);
            lbl->setText(label);
            return lbl;
        };

        auto createGlyphButton = [this](UTF8StringPtr glyph, const CRect& bounds, int tag, int fontsize) {
            STextButton* btn = new STextButton(bounds, this, tag, glyph);
            btn->setFont(makeOwned<CFontDesc>("Sfizz Misc Icons", fontsize));
            btn->setTextColor(kWhiteCColor);
            btn->setHighlightColor(CColor(0xfd, 0x98, 0x00, 0xff));
            btn->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            btn->setFrameColorHighlighted(CColor(0x00, 0x00, 0x00, 0x00));
            btn->setGradient(nullptr);
            btn->setGradientHighlighted(nullptr);
            return btn;
        };
        auto createButtonSfztools = [this, &createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            STextButton* btn = createGlyphButton(u8"\ue000", bounds, tag, fontsize);
            btn->OnHoverEnter = [this, btn]() { buttonHoverEnter(btn, "SFZ Tools"); };
            btn->OnHoverLeave = [this, btn]() { buttonHoverLeave(btn); };
            return btn;
        };
        auto createButtonGithub = [this, &createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            STextButton* btn = createGlyphButton(u8"\ue001", bounds, tag, fontsize);
            btn->OnHoverEnter = [this, btn]() { buttonHoverEnter(btn, "Source code"); };
            btn->OnHoverLeave = [this, btn]() { buttonHoverLeave(btn); };
            return btn;
        };
        auto createButtonDiscord = [this, &createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            STextButton* btn = createGlyphButton(u8"\ue002", bounds, tag, fontsize);
            btn->OnHoverEnter = [this, btn]() { buttonHoverEnter(btn, "Community chat"); };
            btn->OnHoverLeave = [this, btn]() { buttonHoverLeave(btn); };
            return btn;
        };
        auto createButtonOpencollective = [this, &createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            STextButton* btn = createGlyphButton(u8"\ue003", bounds, tag, fontsize);
            btn->OnHoverEnter = [this, btn]() { buttonHoverEnter(btn, "Support us"); };
            btn->OnHoverLeave = [this, btn]() { buttonHoverLeave(btn); };
            return btn;
        };
        auto createButtonSfzformat = [this, &createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            STextButton* btn = createGlyphButton(u8"\ue004", bounds, tag, fontsize);
            btn->OnHoverEnter = [this, btn]() { buttonHoverEnter(btn, "SFZ Format"); };
            btn->OnHoverLeave = [this, btn]() { buttonHoverLeave(btn); };
            return btn;
        };

        #include "layout/about.hpp"
    }
    addView(aboutView);

    CRect aboutBounds = aboutView->getViewSize();
    aboutBounds.centerInside(CRect(bounds).originize());
    aboutView->setViewSize(aboutBounds);

    ///
    sysInfoTemplate_ = lblSysInfoValue_->getText();
    sysInfoVariables_["%Pluginformat%"] = {};
    sysInfoVariables_["%HostOS%"] = getOperatingSystemName();
    sysInfoVariables_["%HostCPU%"] = getProcessorName();
    sysInfoVariables_["%HostBits%"] = std::to_string(8 * sizeof(void*));
    sysInfoVariables_["%HostProgram%"] = getCurrentProcessName();
    updateSysInfo();
}

void SAboutDialog::setPluginFormat(const std::string& pluginFormat)
{
    sysInfoVariables_["%PluginFormat%"] = pluginFormat;
    updateSysInfo();
}

void SAboutDialog::setPluginHost(const std::string& pluginHost)
{
    sysInfoVariables_["%HostProgram%"] = pluginHost;
    updateSysInfo();
}

void SAboutDialog::updateSysInfo()
{
    std::string text = sysInfoTemplate_;
    for (const auto& infoKeyValue : sysInfoVariables_) {
        size_t pos = text.find(infoKeyValue.first);
        if (pos != text.npos)
            text.replace(pos, infoKeyValue.first.size(), infoKeyValue.second);
    }
    lblSysInfoValue_->setText(UTF8String(text));
}

void SAboutDialog::buttonHoverEnter(CControl* btn, const char* text)
{
    CRect rect = lblHover_->getViewSize();
    CRect btnRect = btn->getViewSize();
    rect.left = btnRect.left - 100;
    rect.right = btnRect.right + 100;
    lblHover_->setViewSize(rect);

    lblHover_->setText(text);
    lblHover_->setVisible(true);
    lblHover_->invalid();
}

void SAboutDialog::buttonHoverLeave(CControl* btn)
{
    (void)btn;
    lblHover_->setVisible(false);
}

CMouseEventResult SAboutDialog::onMouseDown(CPoint& where, const CButtonState& buttons)
{
    CMouseEventResult result = CViewContainer::onMouseDown(where, buttons);

    if (result != kMouseEventHandled) {
        setVisible(false);
        result = kMouseEventHandled;
    }

    return result;
}

void SAboutDialog::valueChanged(CControl *ctl)
{
    int32_t tag = ctl->getTag();
    float value = ctl->getValue();

    switch (tag) {
    case kTagButtonSfztools:
        if (value != 1)
            break;

        Call::later([]() {
            openURLWithExternalProgram("https://sfz.tools/sfizz/");
        });
        break;

    case kTagButtonGithub:
        if (value != 1)
            break;

        Call::later([]() {
            openURLWithExternalProgram("https://github.com/sfztools/sfizz");
        });
        break;

    case kTagButtonDiscord:
        if (value != 1)
            break;

        Call::later([]() {
            openURLWithExternalProgram("https://discord.gg/3ArE9Mw");
        });
        break;

    case kTagButtonOpencollective:
        if (value != 1)
            break;

        Call::later([]() {
            openURLWithExternalProgram("https://opencollective.com/sfztools");
        });
        break;

    case kTagButtonSfzformat:
        if (value != 1)
            break;

        Call::later([]() {
            openURLWithExternalProgram("https://sfzformat.com/");
        });
        break;

    default:
        break;
    }
}
