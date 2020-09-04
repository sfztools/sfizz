// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Editor.h"
#include "EditorController.h"
#include "EditIds.h"
#include "GUIComponents.h"
#include <absl/strings/string_view.h>
#include <cstdarg>
#include <cstdio>

#include "utility/vstgui_before.h"
#include "vstgui/vstgui.h"
#include "utility/vstgui_after.h"

using namespace VSTGUI;

const int Editor::viewWidth { 482 };
const int Editor::viewHeight { 225 };

struct Editor::Impl : EditorController::Receiver, IControlListener {
    EditorController* ctrl_ = nullptr;
    CFrame* frame_ = nullptr;
    SharedPointer<CViewContainer> view_;

    enum {
        kPanelGeneral,
        // kPanelControls,
        kPanelSettings,
        kPanelTuning,
        kPanelInfo,
        kNumPanels,
    };

    unsigned activePanel_ = 0;
    CViewContainer* subPanels_[kNumPanels] = {};

    enum {
        kTagLoadSfzFile,
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

    CTextLabel* sfzFileLabel_ = nullptr;
    CTextLabel* scalaFileLabel_ = nullptr;
    CSliderBase *volumeSlider_ = nullptr;
    CTextLabel* volumeLabel_ = nullptr;
    CSliderBase *numVoicesSlider_ = nullptr;
    CTextLabel* numVoicesLabel_ = nullptr;
    CSliderBase *oversamplingSlider_ = nullptr;
    CTextLabel* oversamplingLabel_ = nullptr;
    CSliderBase *preloadSizeSlider_ = nullptr;
    CTextLabel* preloadSizeLabel_ = nullptr;
    CSliderBase *scalaRootKeySlider_ = nullptr;
    CTextLabel* scalaRootKeyLabel_ = nullptr;
    CSliderBase *tuningFrequencySlider_ = nullptr;
    CTextLabel* tuningFrequencyLabel_ = nullptr;
    CSliderBase *stretchedTuningSlider_ = nullptr;
    CTextLabel* stretchedTuningLabel_ = nullptr;

    CTextLabel* infoCurvesLabel_ = nullptr;
    CTextLabel* infoMastersLabel_ = nullptr;
    CTextLabel* infoGroupsLabel_ = nullptr;
    CTextLabel* infoRegionsLabel_ = nullptr;
    CTextLabel* infoSamplesLabel_ = nullptr;
    CTextLabel* infoVoicesLabel_ = nullptr;

    void uiReceiveValue(EditId id, const EditValue& v) override;

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
    void chooseScalaFile();

    void updateSfzFileLabel(const std::string& filePath);
    void updateScalaFileLabel(const std::string& filePath);
    static void updateLabelWithFileName(CTextLabel* label, const std::string& filePath);
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
    frame.addView(impl.view_.get());
}

void Editor::close()
{
    Impl& impl = *impl_;

    if (impl.frame_) {
        impl.frame_->removeView(impl.view_.get(), false);
        impl.frame_ = nullptr;
    }
}

void Editor::Impl::uiReceiveValue(EditId id, const EditValue& v)
{
    switch (id) {
    case EditId::SfzFile:
        {
            const std::string& value = v.to_string();
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
            updateScalaFileLabel(value);
        }
        break;
    case EditId::ScalaRootKey:
        {
            const int value = static_cast<int>(v.to_float());
            if (scalaRootKeySlider_)
                scalaRootKeySlider_->setValue(value);
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

void Editor::Impl::createFrameContents()
{
    const CRect bounds { 0.0, 0.0, static_cast<CCoord>(viewWidth), static_cast<CCoord>(viewHeight) };
    CViewContainer* view = new CViewContainer(bounds);
    view_ = owned(view);

    view->setBackgroundColor(CColor(0xff, 0xff, 0xff));

    SharedPointer<CBitmap> logo = owned(new CBitmap("logo.png"));

    CRect bottomRow = bounds;
    bottomRow.top = bottomRow.bottom - 30;

    CRect topRow = bounds;
    topRow.bottom = topRow.top + 30;

    CViewContainer* panel;
    activePanel_ = 0;

    CRect topLeftLabelBox = topRow;
    topLeftLabelBox.right -= 20 * kNumPanels;

    // general panel
    {
        panel = new CViewContainer(bounds);
        view->addView(panel);
        panel->setTransparency(true);

        CKickButton* sfizzButton = new CKickButton(bounds, this, kTagLoadSfzFile, logo);
        panel->addView(sfizzButton);

        CTextLabel* topLeftLabel = new CTextLabel(topLeftLabelBox, "No file loaded");
        topLeftLabel->setFontColor(CColor(0x00, 0x00, 0x00));
        topLeftLabel->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        panel->addView(topLeftLabel);
        sfzFileLabel_ = topLeftLabel;

        subPanels_[kPanelGeneral] = panel;
    }

    // settings panel
    {
        panel = new CViewContainer(bounds);
        view->addView(panel);
        panel->setTransparency(true);

        CTextLabel* topLeftLabel = new CTextLabel(topLeftLabelBox, "Settings");
        topLeftLabel->setFontColor(CColor(0x00, 0x00, 0x00));
        topLeftLabel->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        panel->addView(topLeftLabel);

        CRect row = topRow;
        row.top += 45.0;
        row.bottom += 45.0;
        row.left += 20.0;
        row.right -= 20.0;

        static const CCoord interRow = 35.0;
        static const CCoord interColumn = 20.0;
        static const int numColumns = 3;

        auto nthColumn = [&row](int colIndex) -> CRect {
            CRect div = row;
            CCoord columnWidth = (div.right - div.left + interColumn) / numColumns - interColumn;
            div.left = div.left + colIndex * (columnWidth + interColumn);
            div.right = div.left + columnWidth;
            return div;
        };

        CTextLabel* label;
        SimpleSlider* slider;

        label = new CTextLabel(nthColumn(0), "Volume");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(nthColumn(1), this, kTagSetVolume);
        panel->addView(slider);
        adjustMinMaxToEditRange(slider, EditId::Volume);
        volumeSlider_ = slider;
        label = new CTextLabel(nthColumn(2), "");
        volumeLabel_ = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(nthColumn(0), "Polyphony");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(nthColumn(1), this, kTagSetNumVoices);
        panel->addView(slider);
        adjustMinMaxToEditRange(slider, EditId::Polyphony);
        numVoicesSlider_ = slider;
        label = new CTextLabel(nthColumn(2), "");
        numVoicesLabel_ = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(nthColumn(0), "Oversampling");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(nthColumn(1), this, kTagSetOversampling);
        panel->addView(slider);
        adjustMinMaxToEditRange(slider, EditId::Oversampling);
        oversamplingSlider_ = slider;
        label = new CTextLabel(nthColumn(2), "");
        oversamplingLabel_ = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(nthColumn(0), "Preload size");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(nthColumn(1), this, kTagSetPreloadSize);
        panel->addView(slider);
        adjustMinMaxToEditRange(slider, EditId::PreloadSize);
        preloadSizeSlider_ = slider;
        label = new CTextLabel(nthColumn(2), "");
        preloadSizeLabel_ = label;
        panel->addView(label);

        subPanels_[kPanelSettings] = panel;
    }

    // tuning panel
    {
        panel = new CViewContainer(bounds);
        view->addView(panel);
        panel->setTransparency(true);

        CTextLabel* topLeftLabel = new CTextLabel(topLeftLabelBox, "Tuning");
        topLeftLabel->setFontColor(CColor(0x00, 0x00, 0x00));
        topLeftLabel->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        panel->addView(topLeftLabel);

        CRect row = topRow;
        row.top += 45.0;
        row.bottom += 45.0;
        row.left += 20.0;
        row.right -= 20.0;

        static const CCoord interRow = 35.0;
        static const CCoord interColumn = 20.0;
        static const int numColumns = 3;

        auto nthColumn = [&row](int colIndex) -> CRect {
            CRect div = row;
            CCoord columnWidth = (div.right - div.left + interColumn) / numColumns - interColumn;
            div.left = div.left + colIndex * (columnWidth + interColumn);
            div.right = div.left + columnWidth;
            return div;
        };

        CTextLabel* label;
        SimpleSlider* slider;
        CTextButton* textbutton;

        label = new CTextLabel(nthColumn(0), "Scala file");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        textbutton = new CTextButton(nthColumn(1), this, kTagLoadScalaFile, "Choose");
        panel->addView(textbutton);
        label = new CTextLabel(nthColumn(2), "");
        scalaFileLabel_ = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(nthColumn(0), "Scala root key");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(nthColumn(1), this, kTagSetScalaRootKey);
        panel->addView(slider);
        adjustMinMaxToEditRange(slider, EditId::ScalaRootKey);
        scalaRootKeySlider_ = slider;
        label = new CTextLabel(nthColumn(2), "");
        scalaRootKeyLabel_ = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(nthColumn(0), "Tuning frequency");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(nthColumn(1), this, kTagSetTuningFrequency);
        panel->addView(slider);
        adjustMinMaxToEditRange(slider, EditId::TuningFrequency);
        tuningFrequencySlider_ = slider;
        label = new CTextLabel(nthColumn(2), "");
        tuningFrequencyLabel_ = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(nthColumn(0), "Stretched tuning");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(nthColumn(1), this, kTagSetStretchedTuning);
        panel->addView(slider);
        adjustMinMaxToEditRange(slider, EditId::StretchTuning);
        stretchedTuningSlider_ = slider;
        label = new CTextLabel(nthColumn(2), "");
        stretchedTuningLabel_ = label;
        panel->addView(label);

        subPanels_[kPanelTuning] = panel;
    }

    // info panel
    {
        panel = new CViewContainer(bounds);
        view->addView(panel);
        panel->setTransparency(true);

        CTextLabel* topLeftLabel = new CTextLabel(topLeftLabelBox, "Information");
        topLeftLabel->setFontColor(CColor(0x00, 0x00, 0x00));
        topLeftLabel->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        panel->addView(topLeftLabel);

        CRect row = topRow;
        row.top += 45.0;
        row.bottom += 45.0;
        row.left += 20.0;
        row.right -= 20.0;

        static const CCoord interRow = 20.0;
        static const CCoord interColumn = 20.0;
        static const int numColumns = 3;

        auto nthColumn = [&row](int colIndex) -> CRect {
            CRect div = row;
            CCoord columnWidth = (div.right - div.left + interColumn) / numColumns - interColumn;
            div.left = div.left + colIndex * (columnWidth + interColumn);
            div.right = div.left + columnWidth;
            return div;
        };

        CTextLabel* label;

        label = new CTextLabel(nthColumn(0), "Curves");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        label = new CTextLabel(nthColumn(1), "");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        infoCurvesLabel_ = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(nthColumn(0), "Masters");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        label = new CTextLabel(nthColumn(1), "");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        infoMastersLabel_ = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(nthColumn(0), "Groups");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        label = new CTextLabel(nthColumn(1), "");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        infoGroupsLabel_ = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(nthColumn(0), "Regions");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        label = new CTextLabel(nthColumn(1), "");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        infoRegionsLabel_ = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(nthColumn(0), "Samples");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        label = new CTextLabel(nthColumn(1), "");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        infoSamplesLabel_ = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(nthColumn(0), "Voices");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        label = new CTextLabel(nthColumn(1), "");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        infoVoicesLabel_ = label;
        panel->addView(label);

        subPanels_[kPanelInfo] = panel;
    }

    // all panels
    for (unsigned currentPanel = 0; currentPanel < kNumPanels; ++currentPanel) {
        panel = subPanels_[currentPanel];

        CTextLabel* descLabel = new CTextLabel(
            bottomRow, "Paul Ferrand and the SFZ Tools work group");
        descLabel->setFontColor(CColor(0x00, 0x00, 0x00));
        descLabel->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        panel->addView(descLabel);

        for (unsigned i = 0; i < kNumPanels; ++i) {
            CRect btnRect = topRow;
            btnRect.left = topRow.right - (kNumPanels - i) * 50;
            btnRect.right = btnRect.left + 50;

            const char *text;
            switch (i) {
            case kPanelGeneral: text = "File"; break;
            case kPanelSettings: text = "Setup"; break;
            case kPanelTuning: text = "Tuning"; break;
            case kPanelInfo: text = "Info"; break;
            default: text = "?"; break;
            }

            CTextButton* changePanelButton = new CTextButton(btnRect, this, kTagFirstChangePanel + i, text);
            panel->addView(changePanelButton);

            changePanelButton->setRoundRadius(0.0);
        }

        panel->setVisible(currentPanel == activePanel_);
    }
}

void Editor::Impl::chooseSfzFile()
{
    SharedPointer<CNewFileSelector> fs = owned(CNewFileSelector::create(frame_));

    fs->setTitle("Load SFZ file");
    fs->setDefaultExtension(CFileExtension("SFZ", "sfz"));

    if (fs->runModal()) {
        UTF8StringPtr file = fs->getSelectedFile(0);
        if (file) {
            std::string str(file);
            ctrl_->uiSendValue(EditId::SfzFile, str);
            updateSfzFileLabel(str);
        }
    }
}

void Editor::Impl::chooseScalaFile()
{
    SharedPointer<CNewFileSelector> fs = owned(CNewFileSelector::create(frame_));

    fs->setTitle("Load Scala file");
    fs->setDefaultExtension(CFileExtension("SCL", "scl"));

    if (fs->runModal()) {
        UTF8StringPtr file = fs->getSelectedFile(0);
        if (file) {
            std::string str(file);
            ctrl_->uiSendValue(EditId::ScalaFile, str);
            updateScalaFileLabel(str);
        }
    }
}

void Editor::Impl::updateSfzFileLabel(const std::string& filePath)
{
    updateLabelWithFileName(sfzFileLabel_, filePath);
}

void Editor::Impl::updateScalaFileLabel(const std::string& filePath)
{
    updateLabelWithFileName(scalaFileLabel_, filePath);
}

void Editor::Impl::updateLabelWithFileName(CTextLabel* label, const std::string& filePath)
{
    if (!label)
        return;

    std::string fileName;
    if (filePath.empty())
        fileName = "<No file>";
    else {
#if defined (_WIN32)
        size_t pos = filePath.find_last_of("/\\");
#else
        size_t pos = filePath.rfind('/');
#endif
        fileName = (pos != filePath.npos) ?
            filePath.substr(pos + 1) : filePath;
    }
    label->setText(fileName.c_str());
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
    sprintf(text, "%.1f kB", preloadSize * (1.0 / 1024));
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
        subPanels_[activePanel_]->setVisible(false);
        activePanel_ = panelId;
        subPanels_[panelId]->setVisible(true);
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
        ctrl.uiSendValue(EditId::ScalaRootKey, value);
        updateScalaRootKeyLabel(static_cast<int>(value));
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
