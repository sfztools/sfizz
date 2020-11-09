// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Editor.h"
#include "EditorController.h"
#include "EditIds.h"
#include "GUIComponents.h"
#include "GUIPiano.h"
#include "NativeHelpers.h"
#include <absl/strings/string_view.h>
#include <absl/strings/match.h>
#include <absl/strings/ascii.h>
#include <ghc/fs_std.hpp>
#include <array>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <system_error>
#include <cstdarg>
#include <cstdio>

#include "utility/vstgui_before.h"
#include "vstgui/vstgui.h"
#include "utility/vstgui_after.h"

using namespace VSTGUI;

const int Editor::viewWidth { 800 };
const int Editor::viewHeight { 475 };

struct Editor::Impl : EditorController::Receiver, IControlListener {
    EditorController* ctrl_ = nullptr;
    CFrame* frame_ = nullptr;
    SharedPointer<CViewContainer> mainView_;

    std::string currentSfzFile_;
    std::string currentScalaFile_;

    enum {
        kPanelGeneral,
        kPanelControls,
        kPanelSettings,
        kNumPanels,
    };

    unsigned activePanel_ = 0;
    CViewContainer* subPanels_[kNumPanels] = {};

    enum {
        kTagLoadSfzFile,
        kTagEditSfzFile,
        kTagPreviousSfzFile,
        kTagNextSfzFile,
        kTagFileOperations,
        kTagSetVolume,
        kTagSetNumVoices,
        kTagSetOversampling,
        kTagSetPreloadSize,
        kTagLoadScalaFile,
        kTagSetScalaRootKey,
        kTagSetTuningFrequency,
        kTagSetStretchedTuning,
        kTagFirstChangePanel,
        kTagLastChangePanel = kTagFirstChangePanel + kNumPanels - 1,
    };

    STextButton* sfzFileLabel_ = nullptr;
    CTextLabel* scalaFileLabel_ = nullptr;
    STextButton* scalaFileButton_ = nullptr;
    CControl *volumeSlider_ = nullptr;
    CTextLabel* volumeLabel_ = nullptr;
    SValueMenu *numVoicesSlider_ = nullptr;
    CTextLabel* numVoicesLabel_ = nullptr;
    SValueMenu *oversamplingSlider_ = nullptr;
    CTextLabel* oversamplingLabel_ = nullptr;
    SValueMenu *preloadSizeSlider_ = nullptr;
    CTextLabel* preloadSizeLabel_ = nullptr;
    SValueMenu *scalaRootKeySlider_ = nullptr;
    SValueMenu *scalaRootOctaveSlider_ = nullptr;
    CTextLabel* scalaRootKeyLabel_ = nullptr;
    SValueMenu *tuningFrequencySlider_ = nullptr;
    CTextLabel* tuningFrequencyLabel_ = nullptr;
    CControl *stretchedTuningSlider_ = nullptr;
    CTextLabel* stretchedTuningLabel_ = nullptr;

    CTextLabel* infoCurvesLabel_ = nullptr;
    CTextLabel* infoMastersLabel_ = nullptr;
    CTextLabel* infoGroupsLabel_ = nullptr;
    CTextLabel* infoRegionsLabel_ = nullptr;
    CTextLabel* infoSamplesLabel_ = nullptr;
    CTextLabel* infoVoicesLabel_ = nullptr;

    CTextLabel* memoryLabel_ = nullptr;

    SActionMenu* fileOperationsMenu_ = nullptr;

    SPiano* piano_ = nullptr;

    void uiReceiveValue(EditId id, const EditValue& v) override;
    void uiReceiveMessage(const char* path, const char* sig, const sfizz_arg_t* args) override;

    void createFrameContents();

    template <class Control>
    void adjustMinMaxToEditRange(Control* c, EditId id)
    {
        const EditRange er = EditRange::get(id);
        c->setMin(er.min);
        c->setMax(er.max);
        c->setDefaultValue(er.def);
    }

    void chooseSfzFile();
    void changeSfzFile(const std::string& filePath);
    void changeToNextSfzFile(long offset);
    void chooseScalaFile();
    void changeScalaFile(const std::string& filePath);

    static bool scanDirectoryFiles(const fs::path& dirPath, std::function<bool(const fs::path&)> filter, std::vector<fs::path>& fileNames);

    static absl::string_view simplifiedFileName(absl::string_view path, absl::string_view removedSuffix, absl::string_view ifEmpty);

    void updateSfzFileLabel(const std::string& filePath);
    void updateScalaFileLabel(const std::string& filePath);
    static void updateLabelWithFileName(CTextLabel* label, const std::string& filePath, absl::string_view removedSuffix);
    static void updateButtonWithFileName(STextButton* button, const std::string& filePath, absl::string_view removedSuffix);
    static void updateSButtonWithFileName(STextButton* button, const std::string& filePath, absl::string_view removedSuffix);
    void updateVolumeLabel(float volume);
    void updateNumVoicesLabel(int numVoices);
    void updateOversamplingLabel(int oversamplingLog2);
    void updatePreloadSizeLabel(int preloadSize);
    void updateScalaRootKeyLabel(int rootKey);
    void updateTuningFrequencyLabel(float tuningFrequency);
    void updateStretchedTuningLabel(float stretchedTuning);

    void setActivePanel(unsigned panelId);

    static void formatLabel(CTextLabel* label, const char* fmt, ...);
    static void vformatLabel(CTextLabel* label, const char* fmt, va_list ap);

    // IControlListener
    void valueChanged(CControl* ctl) override;
    void enterOrLeaveEdit(CControl* ctl, bool enter);
    void controlBeginEdit(CControl* ctl) override;
    void controlEndEdit(CControl* ctl) override;
};

Editor::Editor(EditorController& ctrl)
    : impl_(new Impl)
{
    Impl& impl = *impl_;

    impl.ctrl_ = &ctrl;

    ctrl.decorate(&impl);

    impl.createFrameContents();
}

Editor::~Editor()
{
    Impl& impl = *impl_;

    close();

    EditorController& ctrl = *impl.ctrl_;
    ctrl.decorate(nullptr);
}

void Editor::open(CFrame& frame)
{
    Impl& impl = *impl_;

    impl.frame_ = &frame;
    frame.addView(impl.mainView_.get());
}

void Editor::close()
{
    Impl& impl = *impl_;

    if (impl.frame_) {
        impl.frame_->removeView(impl.mainView_.get(), false);
        impl.frame_ = nullptr;
    }
}

void Editor::Impl::uiReceiveValue(EditId id, const EditValue& v)
{
    switch (id) {
    case EditId::SfzFile:
        {
            const std::string& value = v.to_string();
            currentSfzFile_ = value;
            updateSfzFileLabel(value);
        }
        break;
    case EditId::Volume:
        {
            const float value = v.to_float();
            if (volumeSlider_)
                volumeSlider_->setValue(value);
            updateVolumeLabel(value);
        }
        break;
    case EditId::Polyphony:
        {
            const int value = static_cast<int>(v.to_float());
            if (numVoicesSlider_)
                numVoicesSlider_->setValue(value);
            updateNumVoicesLabel(value);
        }
        break;
    case EditId::Oversampling:
        {
            const int value = static_cast<int>(v.to_float());

            int log2Value = 0;
            for (int f = value; f > 1; f /= 2)
                ++log2Value;

            if (oversamplingSlider_)
                oversamplingSlider_->setValue(log2Value);
            updateOversamplingLabel(log2Value);
        }
        break;
    case EditId::PreloadSize:
        {
            const int value = static_cast<int>(v.to_float());
            if (preloadSizeSlider_)
                preloadSizeSlider_->setValue(value);
            updatePreloadSizeLabel(value);
        }
        break;
    case EditId::ScalaFile:
        {
            const std::string& value = v.to_string();
            currentScalaFile_ = value;
            updateScalaFileLabel(value);
        }
        break;
    case EditId::ScalaRootKey:
        {
            const int value = std::max(0, static_cast<int>(v.to_float()));
            if (scalaRootKeySlider_)
                scalaRootKeySlider_->setValue(value % 12);
            if (scalaRootOctaveSlider_)
                scalaRootOctaveSlider_->setValue(value / 12);
            updateScalaRootKeyLabel(value);
        }
        break;
    case EditId::TuningFrequency:
        {
            const float value = v.to_float();
            if (tuningFrequencySlider_)
                tuningFrequencySlider_->setValue(value);
            updateTuningFrequencyLabel(value);
        }
        break;
    case EditId::StretchTuning:
        {
            const float value = v.to_float();
            if (stretchedTuningSlider_)
                stretchedTuningSlider_->setValue(value);
            updateStretchedTuningLabel(value);
        }
        break;
    case EditId::UINumCurves:
        {
            const int value = static_cast<int>(v.to_float());
            if (CTextLabel* label = infoCurvesLabel_)
                formatLabel(label, "%u", value);
        }
        break;
    case EditId::UINumMasters:
        {
            const int value = static_cast<int>(v.to_float());
            if (CTextLabel* label = infoMastersLabel_)
                formatLabel(label, "%u", value);
        }
        break;
    case EditId::UINumGroups:
        {
            const int value = static_cast<int>(v.to_float());
            if (CTextLabel* label = infoGroupsLabel_)
                formatLabel(label, "%u", value);
        }
        break;
    case EditId::UINumRegions:
        {
            const int value = static_cast<int>(v.to_float());
            if (CTextLabel* label = infoRegionsLabel_)
                formatLabel(label, "%u", value);
        }
        break;
    case EditId::UINumPreloadedSamples:
        {
            const int value = static_cast<int>(v.to_float());
            if (CTextLabel* label = infoSamplesLabel_)
                formatLabel(label, "%u", value);
        }
        break;
    case EditId::UINumActiveVoices:
        {
            const int value = static_cast<int>(v.to_float());
            if (CTextLabel* label = infoVoicesLabel_)
                formatLabel(label, "%u", value);
        }
        break;
    case EditId::UIActivePanel:
        {
            const int value = static_cast<int>(v.to_float());
            setActivePanel(value);
        }
        break;
    }
}

void Editor::Impl::uiReceiveMessage(const char* path, const char* sig, const sfizz_arg_t* args)
{
    // TODO handle the message...
}

void Editor::Impl::createFrameContents()
{
    CViewContainer* mainView;

    SharedPointer<CBitmap> iconWhite = owned(new CBitmap("logo_text_white.png"));
    SharedPointer<CBitmap> background = owned(new CBitmap("background.png"));
    SharedPointer<CBitmap> knob48 = owned(new CBitmap("knob48.png"));
    SharedPointer<CBitmap> logoText = owned(new CBitmap("logo_text.png"));

    {
        const CColor frameBackground = { 0xd3, 0xd7, 0xcf };

        struct Theme {
            CColor boxBackground;
            CColor text;
            CColor inactiveText;
            CColor highlightedText;
            CColor titleBoxText;
            CColor titleBoxBackground;
            CColor icon;
            CColor iconHighlight;
            CColor valueText;
            CColor valueBackground;
            CColor knobActiveTrackColor;
            CColor knobInactiveTrackColor;
            CColor knobLineIndicatorColor;
        };

        Theme lightTheme;
        lightTheme.boxBackground = { 0xba, 0xbd, 0xb6 };
        lightTheme.text = { 0x00, 0x00, 0x00 };
        lightTheme.inactiveText = { 0xb2, 0xb2, 0xb2 };
        lightTheme.highlightedText = { 0xfd, 0x98, 0x00 };
        lightTheme.titleBoxText = { 0xff, 0xff, 0xff };
        lightTheme.titleBoxBackground = { 0x2e, 0x34, 0x36 };
        lightTheme.icon = lightTheme.text;
        lightTheme.iconHighlight = { 0xfd, 0x98, 0x00 };
        lightTheme.valueText = { 0xff, 0xff, 0xff };
        lightTheme.valueBackground = { 0x2e, 0x34, 0x36 };
        lightTheme.knobActiveTrackColor = { 0x00, 0xb6, 0x2a };
        lightTheme.knobInactiveTrackColor = { 0x30, 0x30, 0x30 };
        lightTheme.knobLineIndicatorColor = { 0x00, 0x00, 0x00 };
        Theme darkTheme;
        darkTheme.boxBackground = { 0x2e, 0x34, 0x36 };
        darkTheme.text = { 0xff, 0xff, 0xff };
        darkTheme.inactiveText = { 0xb2, 0xb2, 0xb2 };
        darkTheme.highlightedText = { 0xfd, 0x98, 0x00 };
        darkTheme.titleBoxText = { 0x00, 0x00, 0x00 };
        darkTheme.titleBoxBackground = { 0xba, 0xbd, 0xb6 };
        darkTheme.icon = darkTheme.text;
        darkTheme.iconHighlight = { 0xfd, 0x98, 0x00 };
        darkTheme.valueText = { 0x2e, 0x34, 0x36 };
        darkTheme.valueBackground = { 0xff, 0xff, 0xff };
        darkTheme.knobActiveTrackColor = { 0x00, 0xb6, 0x2a };
        darkTheme.knobInactiveTrackColor = { 0x60, 0x60, 0x60 };
        darkTheme.knobLineIndicatorColor = { 0xff, 0xff, 0xff };
        Theme& defaultTheme = lightTheme;

        Theme* theme = &defaultTheme;
        auto enterTheme = [&theme](Theme& t) { theme = &t; };

        typedef CViewContainer LogicalGroup;
        typedef SBoxContainer RoundedGroup;
        typedef STitleContainer TitleGroup;
        typedef CKickButton SfizzMainButton;
        typedef CTextLabel Label;
        typedef CViewContainer HLine;
        typedef CAnimKnob Knob48;
        typedef SStyledKnob StyledKnob;
        typedef CTextLabel ValueLabel;
        typedef CViewContainer VMeter;
        typedef SValueMenu ValueMenu;
        typedef CViewContainer Background;
#if 0
        typedef CTextButton Button;
#endif
        typedef STextButton ClickableLabel;
        typedef STextButton ValueButton;
        typedef STextButton LoadFileButton;
        typedef STextButton CCButton;
        typedef STextButton HomeButton;
        typedef STextButton SettingsButton;
        typedef STextButton EditFileButton;
        typedef STextButton PreviousFileButton;
        typedef STextButton NextFileButton;
        typedef SPiano Piano;
        typedef SActionMenu ChevronDropDown;

        auto createLogicalGroup = [](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            CViewContainer* container = new CViewContainer(bounds);
            container->setBackgroundColor(CColor(0x00, 0x00, 0x00, 0x00));
            return container;
        };
        auto createRoundedGroup = [&theme](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            auto* box =  new SBoxContainer(bounds);
            box->setCornerRadius(10.0);
            box->setBackgroundColor(theme->boxBackground);
            return box;
        };
        auto createTitleGroup = [&theme](const CRect& bounds, int, const char* label, CHoriTxtAlign, int fontsize) {
            auto* box =  new STitleContainer(bounds, label);
            box->setCornerRadius(10.0);
            box->setBackgroundColor(theme->boxBackground);
            box->setTitleFontColor(theme->titleBoxText);
            box->setTitleBackgroundColor(theme->titleBoxBackground);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            box->setTitleFont(font);
            return box;
        };
        auto createSfizzMainButton = [this, &iconWhite](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int) {
            return new CKickButton(bounds, this, tag, iconWhite);
        };
        auto createLabel = [&theme](const CRect& bounds, int, const char* label, CHoriTxtAlign align, int fontsize) {
            CTextLabel* lbl = new CTextLabel(bounds, label);
            lbl->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setFontColor(theme->text);
            lbl->setHoriAlign(align);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            lbl->setFont(font);
            return lbl;
        };
        auto createHLine = [](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            int y = static_cast<int>(0.5 * (bounds.top + bounds.bottom));
            CRect lineBounds(bounds.left, y, bounds.right, y + 1);
            CViewContainer* hline = new CViewContainer(lineBounds);
            hline->setBackgroundColor(CColor(0xff, 0xff, 0xff, 0xff));
            return hline;
        };
        auto createKnob48 = [this, &knob48](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int) {
            return new CAnimKnob(bounds, this, tag, 31, 48, knob48);
        };
        auto createStyledKnob = [this, &theme](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int) {
            SStyledKnob* knob = new SStyledKnob(bounds, this, tag);
            knob->setActiveTrackColor(theme->knobActiveTrackColor);
            knob->setInactiveTrackColor(theme->knobInactiveTrackColor);
            knob->setLineIndicatorColor(theme->knobLineIndicatorColor);
            return knob;
        };
        auto createValueLabel = [&theme](const CRect& bounds, int, const char* label, CHoriTxtAlign align, int fontsize) {
            CTextLabel* lbl = new CTextLabel(bounds, label);
            lbl->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setFontColor(theme->text);
            lbl->setHoriAlign(align);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            lbl->setFont(font);
            return lbl;
        };
        auto createVMeter = [](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            // TODO the volume meter...
            CViewContainer* container = new CViewContainer(bounds);
            container->setBackgroundColor(CColor(0x00, 0x00, 0x00, 0x00));
            return container;
        };
#if 0
        auto createButton = [this](const CRect& bounds, int tag, const char* label, CHoriTxtAlign align, int fontsize) {
            CTextButton* button = new CTextButton(bounds, this, tag, label);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            button->setFont(font);
            button->setTextAlignment(align);
            return button;
        };
#endif
        auto createClickableLabel = [this, &theme](const CRect& bounds, int tag, const char* label, CHoriTxtAlign align, int fontsize) {
            STextButton* button = new STextButton(bounds, this, tag, label);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            button->setFont(font);
            button->setTextAlignment(align);
            button->setTextColor(theme->text);
            button->setInactiveColor(theme->inactiveText);
            button->setHoverColor(theme->highlightedText);
            button->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            button->setFrameColorHighlighted(CColor(0x00, 0x00, 0x00, 0x00));
            SharedPointer<CGradient> gradient = owned(CGradient::create(0.0, 1.0, CColor(0x00, 0x00, 0x00, 0x00), CColor(0x00, 0x00, 0x00, 0x00)));
            button->setGradient(gradient);
            button->setGradientHighlighted(gradient);
            return button;
        };
        auto createValueButton = [this, &theme](const CRect& bounds, int tag, const char* label, CHoriTxtAlign align, int fontsize) {
            STextButton* button = new STextButton(bounds, this, tag, label);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            button->setFont(font);
            button->setTextAlignment(align);
            button->setTextColor(theme->valueText);
            button->setInactiveColor(theme->inactiveText);
            button->setHoverColor(theme->highlightedText);
            button->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            button->setFrameColorHighlighted(CColor(0x00, 0x00, 0x00, 0x00));
            SharedPointer<CGradient> gradient = owned(CGradient::create(0.0, 1.0, theme->valueBackground, theme->valueBackground));
            button->setGradient(gradient);
            button->setGradientHighlighted(gradient);
            return button;
        };
        auto createValueMenu = [this, &theme](const CRect& bounds, int tag, const char*, CHoriTxtAlign align, int fontsize) {
            SValueMenu* vm = new SValueMenu(bounds, this, tag);
            vm->setHoriAlign(align);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            vm->setFont(font);
            vm->setFontColor(theme->valueText);
            vm->setBackColor(theme->valueBackground);
            vm->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            vm->setStyle(CParamDisplay::kRoundRectStyle);
            vm->setRoundRectRadius(5.0);
            return vm;
        };
        auto createGlyphButton = [this, &theme](UTF8StringPtr glyph, const CRect& bounds, int tag, int fontsize) {
            STextButton* btn = new STextButton(bounds, this, tag, glyph);
            btn->setFont(makeOwned<CFontDesc>("Sfizz Fluent System R20", fontsize));
            btn->setTextColor(theme->icon);
            btn->setHoverColor(theme->iconHighlight);
            btn->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            btn->setFrameColorHighlighted(CColor(0x00, 0x00, 0x00, 0x00));
            btn->setGradient(nullptr);
            btn->setGradientHighlighted(nullptr);
            return btn;
        };
        auto createHomeButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            return createGlyphButton(u8"\ue1d6", bounds, tag, fontsize);
        };
        auto createCCButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            // return createGlyphButton(u8"\ue240", bounds, tag, fontsize);
            return createGlyphButton(u8"\ue253", bounds, tag, fontsize);
        };
        auto createSettingsButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            return createGlyphButton(u8"\ue2e4", bounds, tag, fontsize);
        };
        auto createEditFileButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            return createGlyphButton(u8"\ue148", bounds, tag, fontsize);
        };
        auto createLoadFileButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            return createGlyphButton(u8"\ue1a3", bounds, tag, fontsize);
        };
        auto createPreviousFileButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            return createGlyphButton(u8"\ue0d9", bounds, tag, fontsize);
        };
        auto createNextFileButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            return createGlyphButton(u8"\ue0da", bounds, tag, fontsize);
        };
        auto createPiano = [](const CRect& bounds, int, const char*, CHoriTxtAlign, int fontsize) {
            SPiano* piano = new SPiano(bounds);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            piano->setFont(font);
            return piano;
        };
        auto createChevronDropDown = [this, &theme](const CRect& bounds, int, const char*, CHoriTxtAlign, int fontsize) {
            SActionMenu* menu = new SActionMenu(bounds, this);
            menu->setTitle(u8"\ue0d7");
            menu->setFont(makeOwned<CFontDesc>("Sfizz Fluent System R20", fontsize));
            menu->setFontColor(theme->icon);
            menu->setHoverColor(theme->iconHighlight);
            menu->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            menu->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            return menu;
        };
        auto createBackground = [&background](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            CViewContainer* container = new CViewContainer(bounds);
            container->setBackground(background);
            return container;
        };

        #include "layout/main.hpp"

        mainView->setBackgroundColor(frameBackground);

        mainView_ = owned(mainView);
    }

    ///
    SharedPointer<SFileDropTarget> fileDropTarget = owned(new SFileDropTarget);

    fileDropTarget->setFileDropFunction([this](const std::string& file) {
        changeSfzFile(file);
    });

    mainView_->setDropTarget(fileDropTarget);

    ///
    adjustMinMaxToEditRange(volumeSlider_, EditId::Volume);
    adjustMinMaxToEditRange(numVoicesSlider_, EditId::Polyphony);
    adjustMinMaxToEditRange(oversamplingSlider_, EditId::Oversampling);
    adjustMinMaxToEditRange(preloadSizeSlider_, EditId::PreloadSize);
    if (scalaRootKeySlider_) {
        scalaRootKeySlider_->setMin(0.0);
        scalaRootKeySlider_->setMax(11.0);
        scalaRootKeySlider_->setDefaultValue(
            static_cast<int>(EditRange::get(EditId::ScalaRootKey).def) % 12);
    }
    if (scalaRootOctaveSlider_) {
        scalaRootOctaveSlider_->setMin(0.0);
        scalaRootOctaveSlider_->setMax(10.0);
        scalaRootOctaveSlider_->setDefaultValue(
            static_cast<int>(EditRange::get(EditId::ScalaRootKey).def) / 12);
    }
    adjustMinMaxToEditRange(tuningFrequencySlider_, EditId::TuningFrequency);
    adjustMinMaxToEditRange(stretchedTuningSlider_, EditId::StretchTuning);

    for (int value : {1, 2, 4, 8, 16, 32, 64, 96, 128, 160, 192, 224, 256})
        numVoicesSlider_->addEntry(std::to_string(value), value);
    numVoicesSlider_->setValueToStringFunction2(
        [](float value, std::string& result, CParamDisplay*) -> bool
        {
            result = std::to_string(static_cast<int32_t>(value));
            return true;
        });

    for (int log2value = 0; log2value <= 3; ++log2value) {
        int value = 1 << log2value;
        oversamplingSlider_->addEntry(std::to_string(value) + "x", log2value);
    }
    oversamplingSlider_->setValueToStringFunction2(
        [](float value, std::string& result, CParamDisplay*) -> bool
        {
            result = std::to_string(1 << static_cast<int32_t>(value)) + "x";
            return true;
        });

    for (int log2value = 10; log2value <= 16; ++log2value) {
        int value = 1 << log2value;
        char text[256];
        sprintf(text, "%lu kB", static_cast<unsigned long>(value / 1024 * sizeof(float)));
        text[sizeof(text) - 1] = '\0';
        preloadSizeSlider_->addEntry(text, value);
    }
    preloadSizeSlider_->setValueToStringFunction2(
        [](float value, std::string& result, CParamDisplay*) -> bool
        {
            result = std::to_string(static_cast<int>(std::round(value * (1.0 / 1024 * sizeof(float))))) + " kB";
            return true;
        });

    static const std::pair<float, const char*> tuningFrequencies[] = {
        {380.0f, "English pitchpipe 380 (1720)"},
        {409.0f, "Handel fork 409 (1780)"},
        {415.0f, "Baroque 415"},
        {422.5f, "Handel fork 422.5 (1740)"},
        {423.2f, "Dresden opera 423.2 (1815)"},
        {435.0f, "French Law 435 (1859)"},
        {439.0f, "British Phil 439 (1896)"},
        {440.0f, "International 440"},
        {442.0f, "European 442"},
        {445.0f, "Germany, China 445"},
        {451.0f, "La Scala in Milan 451 (18th)"},
    };

    for (std::pair<float, const char*> value : tuningFrequencies)
        tuningFrequencySlider_->addEntry(value.second, value.first);
    tuningFrequencySlider_->setValueToStringFunction(
        [](float value, char result[256], CParamDisplay*) -> bool
        {
            sprintf(result, "%.1f Hz", value);
            return true;
        });

    static const char* notesInOctave[12] = {
        "C", "C#", "D", "D#", "E",
        "F", "F#", "G", "G#", "A", "A#", "B",
    };
    for (int note = 0; note < 12; ++note)
        scalaRootKeySlider_->addEntry(notesInOctave[note], note);
    for (int octave = 0; octave <= 10; ++octave)
        scalaRootOctaveSlider_->addEntry(std::to_string(octave - 1), octave);
    scalaRootKeySlider_->setValueToStringFunction2(
        [](float value, std::string& result, CParamDisplay*) -> bool
        {
            result = notesInOctave[std::max(0, static_cast<int>(value)) % 12];
            return true;
        });
    scalaRootOctaveSlider_->setValueToStringFunction2(
        [](float value, std::string& result, CParamDisplay*) -> bool
        {
            result = std::to_string(static_cast<int>(value) - 1);
            return true;
        });

    if (SActionMenu* menu = fileOperationsMenu_) {
        menu->addEntry("Load file", kTagLoadSfzFile);
        menu->addEntry("Edit file", kTagEditSfzFile);
    }

    if (SPiano* piano = piano_) {
        piano->onKeyPressed = [this](unsigned key, float vel) {
            uint8_t msg[3];
            msg[0] = 0x90;
            msg[1] = static_cast<uint8_t>(key);
            msg[2] = static_cast<uint8_t>(std::max(1, static_cast<int>(vel * 127)));
            ctrl_->uiSendMIDI(msg, sizeof(msg));
        };
        piano->onKeyReleased = [this](unsigned key, float vel) {
            uint8_t msg[3];
            msg[0] = 0x80;
            msg[1] = static_cast<uint8_t>(key);
            msg[2] = static_cast<uint8_t>(vel * 127);
            ctrl_->uiSendMIDI(msg, sizeof(msg));
        };
    }

    ///
    CViewContainer* panel;
    activePanel_ = 0;

    // all panels
    for (unsigned currentPanel = 0; currentPanel < kNumPanels; ++currentPanel) {
        panel = subPanels_[currentPanel];

        if (!panel)
            continue;

        panel->setVisible(currentPanel == activePanel_);
    }
}

void Editor::Impl::chooseSfzFile()
{
    SharedPointer<CNewFileSelector> fs = owned(CNewFileSelector::create(frame_));

    fs->setTitle("Load SFZ file");
    fs->setDefaultExtension(CFileExtension("SFZ", "sfz"));
    if (!currentSfzFile_.empty()) {
        std::string initialDir = fs::path(currentSfzFile_).parent_path().u8string() + '/';
        fs->setInitialDirectory(initialDir.c_str());
    }

    if (fs->runModal()) {
        UTF8StringPtr file = fs->getSelectedFile(0);
        if (file)
            changeSfzFile(file);
    }
}

void Editor::Impl::changeSfzFile(const std::string& filePath)
{
    ctrl_->uiSendValue(EditId::SfzFile, filePath);
    currentSfzFile_ = filePath;
    updateSfzFileLabel(filePath);
}

void Editor::Impl::changeToNextSfzFile(long offset)
{
    if (currentSfzFile_.empty())
        return;

    const fs::path filePath = fs::u8path(currentSfzFile_);
    const fs::path dirPath = filePath.parent_path();

    // extract file names of regular files from the sfz directory
    std::vector<fs::path> fileNames;
    fileNames.reserve(64);

    auto fileFilter = [](const fs::path &name) -> bool {
        std::string ext = name.extension().u8string();
        absl::AsciiStrToLower(&ext);
        return ext == ".sfz";
    };

    if (!scanDirectoryFiles(dirPath, fileFilter, fileNames))
        return;

    // sort file names
    const size_t size = fileNames.size();
    if (size == 0)
        return;

    std::sort(fileNames.begin(), fileNames.end());

    // find our current position in the file name list
    size_t currentIndex = 0;
    const fs::path currentFileName = filePath.filename();

    while (currentIndex + 1 < size && fileNames[currentIndex] < currentFileName)
        ++currentIndex;

    // advance to the next or previous item
    typedef typename std::make_signed<size_t>::type signed_size_t;

    size_t newIndex = static_cast<signed_size_t>(currentIndex) + offset;
    if (static_cast<signed_size_t>(newIndex) < 0)
        newIndex = static_cast<signed_size_t>(newIndex) %
            static_cast<signed_size_t>(size) + size;
    newIndex %= size;

    if (newIndex != currentIndex) {
        const fs::path newFilePath = dirPath / fileNames[newIndex];
        changeSfzFile(newFilePath.u8string());
    }
}

void Editor::Impl::chooseScalaFile()
{
    SharedPointer<CNewFileSelector> fs = owned(CNewFileSelector::create(frame_));

    fs->setTitle("Load Scala file");
    fs->setDefaultExtension(CFileExtension("SCL", "scl"));
    if (!currentScalaFile_.empty()) {
        std::string initialDir = fs::path(currentScalaFile_).parent_path().u8string() + '/';
        fs->setInitialDirectory(initialDir.c_str());
    }

    if (fs->runModal()) {
        UTF8StringPtr file = fs->getSelectedFile(0);
        if (file)
            changeScalaFile(file);
    }
}

void Editor::Impl::changeScalaFile(const std::string& filePath)
{
    ctrl_->uiSendValue(EditId::ScalaFile, filePath);
    currentScalaFile_ = filePath;
    updateScalaFileLabel(filePath);
}

bool Editor::Impl::scanDirectoryFiles(const fs::path& dirPath, std::function<bool(const fs::path&)> filter, std::vector<fs::path>& fileNames)
{
    std::error_code ec;
    fs::directory_iterator it { dirPath, ec };

    if (ec)
        return false;

    fileNames.clear();

    while (!ec && it != fs::directory_iterator()) {
        const fs::directory_entry& ent = *it;

        std::error_code fileEc;
        const fs::file_status status = ent.status(fileEc);
        if (fileEc)
            continue;

        if (status.type() == fs::file_type::regular) {
            fs::path fileName = ent.path().filename();
            if (!filter || filter(fileName))
                fileNames.push_back(std::move(fileName));
        }

        it.increment(ec);
    }

    if (ec)
        return false;

    return true;
}

absl::string_view Editor::Impl::simplifiedFileName(absl::string_view path, absl::string_view removedSuffix, absl::string_view ifEmpty)
{
    if (path.empty())
        return ifEmpty;

#if defined (_WIN32)
    size_t pos = path.find_last_of("/\\");
#else
    size_t pos = path.rfind('/');
#endif
    path = (pos != path.npos) ? path.substr(pos + 1) : path;

    if (!removedSuffix.empty() && absl::EndsWithIgnoreCase(path, removedSuffix))
        path.remove_suffix(removedSuffix.size());

    return path;
}

void Editor::Impl::updateSfzFileLabel(const std::string& filePath)
{
    updateButtonWithFileName(sfzFileLabel_, filePath, ".sfz");
}

void Editor::Impl::updateScalaFileLabel(const std::string& filePath)
{
    updateLabelWithFileName(scalaFileLabel_, filePath, ".scl");
    updateButtonWithFileName(scalaFileButton_, filePath, ".scl");
}

void Editor::Impl::updateLabelWithFileName(CTextLabel* label, const std::string& filePath, absl::string_view removedSuffix)
{
    if (!label)
        return;

    std::string fileName = std::string(simplifiedFileName(filePath, removedSuffix, "<No file>"));
    label->setText(fileName.c_str());
}

void Editor::Impl::updateButtonWithFileName(STextButton* button, const std::string& filePath, absl::string_view removedSuffix)
{
    if (!button)
        return;

    std::string fileName = std::string(simplifiedFileName(filePath, removedSuffix, {}));
    if (!fileName.empty()) {
        button->setTitle(fileName.c_str());
        button->setInactive(false);
    }
    else {
        button->setTitle("No file");
        button->setInactive(true);
    }
}

void Editor::Impl::updateVolumeLabel(float volume)
{
    CTextLabel* label = volumeLabel_;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%.1f dB", volume);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::updateNumVoicesLabel(int numVoices)
{
    CTextLabel* label = numVoicesLabel_;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%d", numVoices);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::updateOversamplingLabel(int oversamplingLog2)
{
    CTextLabel* label = oversamplingLabel_;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%dx", 1 << oversamplingLog2);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::updatePreloadSizeLabel(int preloadSize)
{
    CTextLabel* label = preloadSizeLabel_;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%d kB", static_cast<int>(std::round(preloadSize * (1.0 / 1024))));
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::updateScalaRootKeyLabel(int rootKey)
{
    CTextLabel* label = scalaRootKeyLabel_;
    if (!label)
        return;

    static const char *octNoteNames[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
    };

    auto noteName = [](int key) -> std::string
    {
        int octNum;
        int octNoteNum;
        if (key >= 0) {
            octNum = key / 12 - 1;
            octNoteNum = key % 12;
        }
        else {
            octNum = -2 - (key + 1) / -12;
            octNoteNum = (key % 12 + 12) % 12;
        }
        return std::string(octNoteNames[octNoteNum]) + std::to_string(octNum);
    };

    label->setText(noteName(rootKey));
}

void Editor::Impl::updateTuningFrequencyLabel(float tuningFrequency)
{
    CTextLabel* label = tuningFrequencyLabel_;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%.1f", tuningFrequency);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::updateStretchedTuningLabel(float stretchedTuning)
{
    CTextLabel* label = stretchedTuningLabel_;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%.3f", stretchedTuning);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::setActivePanel(unsigned panelId)
{
    panelId = std::max(0, std::min(kNumPanels - 1, static_cast<int>(panelId)));

    if (activePanel_ != panelId) {
        if (subPanels_[activePanel_])
            subPanels_[activePanel_]->setVisible(false);
        if (subPanels_[panelId])
            subPanels_[panelId]->setVisible(true);
        activePanel_ = panelId;
    }
}

void Editor::Impl::formatLabel(CTextLabel* label, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vformatLabel(label, fmt, ap);
    va_end(ap);
}

void Editor::Impl::vformatLabel(CTextLabel* label, const char* fmt, va_list ap)
{
    char text[256];
    vsprintf(text, fmt, ap);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::valueChanged(CControl* ctl)
{
    int32_t tag = ctl->getTag();
    float value = ctl->getValue();
    EditorController& ctrl = *ctrl_;

    switch (tag) {
    case kTagLoadSfzFile:
        if (value != 1)
            break;

        Call::later([this]() { chooseSfzFile(); });
        break;

    case kTagEditSfzFile:
        if (value != 1)
            break;

        if (!currentSfzFile_.empty())
            openFileInExternalEditor(currentSfzFile_.c_str());
        break;

    case kTagPreviousSfzFile:
        if (value != 1)
            break;

        Call::later([this]() { changeToNextSfzFile(-1); });
        break;

    case kTagNextSfzFile:
        if (value != 1)
            break;

        Call::later([this]() { changeToNextSfzFile(+1); });
        break;

    case kTagLoadScalaFile:
        if (value != 1)
            break;

        Call::later([this]() { chooseScalaFile(); });
        break;

    case kTagSetVolume:
        ctrl.uiSendValue(EditId::Volume, value);
        updateVolumeLabel(value);
        break;

    case kTagSetNumVoices:
        ctrl.uiSendValue(EditId::Polyphony, value);
        updateNumVoicesLabel(static_cast<int>(value));
        break;

    case kTagSetOversampling:
        ctrl.uiSendValue(EditId::Oversampling, static_cast<float>(1 << static_cast<int>(value)));
        updateOversamplingLabel(static_cast<int>(value));
        break;

    case kTagSetPreloadSize:
        ctrl.uiSendValue(EditId::PreloadSize, value);
        updatePreloadSizeLabel(static_cast<int>(value));
        break;

    case kTagSetScalaRootKey:
        {
            if (scalaRootKeySlider_ && scalaRootOctaveSlider_) {
                int key = static_cast<int>(scalaRootKeySlider_->getValue());
                int octave = static_cast<int>(scalaRootOctaveSlider_->getValue());
                int midiKey = key + 12 * octave;
                ctrl.uiSendValue(EditId::ScalaRootKey, midiKey);
                updateScalaRootKeyLabel(midiKey);
            }
        }
        break;

    case kTagSetTuningFrequency:
        ctrl.uiSendValue(EditId::TuningFrequency, value);
        updateTuningFrequencyLabel(value);
        break;

    case kTagSetStretchedTuning:
        ctrl.uiSendValue(EditId::StretchTuning, value);
        updateStretchedTuningLabel(value);
        break;

    default:
        if (tag >= kTagFirstChangePanel && tag <= kTagLastChangePanel) {
            int panelId = tag - kTagFirstChangePanel;
            ctrl.uiSendValue(EditId::UIActivePanel, static_cast<float>(panelId));
            setActivePanel(panelId);
        }
        break;
    }
}

void Editor::Impl::enterOrLeaveEdit(CControl* ctl, bool enter)
{
    int32_t tag = ctl->getTag();
    EditId id;

    switch (tag) {
    case kTagSetVolume: id = EditId::Volume; break;
    case kTagSetNumVoices: id = EditId::Polyphony; break;
    case kTagSetOversampling: id = EditId::Oversampling; break;
    case kTagSetPreloadSize: id = EditId::PreloadSize; break;
    case kTagSetScalaRootKey: id = EditId::ScalaRootKey; break;
    case kTagSetTuningFrequency: id = EditId::TuningFrequency; break;
    case kTagSetStretchedTuning: id = EditId::StretchTuning; break;
    default: return;
    }

    EditorController& ctrl = *ctrl_;
    if (enter)
        ctrl.uiBeginSend(id);
    else
        ctrl.uiEndSend(id);
}

void Editor::Impl::controlBeginEdit(CControl* ctl)
{
    enterOrLeaveEdit(ctl, true);
}

void Editor::Impl::controlEndEdit(CControl* ctl)
{
    enterOrLeaveEdit(ctl, false);
}
