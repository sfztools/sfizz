// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstController.h"
#include "public.sdk/source/vst/vstguieditor.h"

using namespace Steinberg;
using namespace VSTGUI;

class SfizzVstEditor : public Vst::VSTGUIEditor, public IControlListener, public SfizzVstController::StateListener {
public:
    explicit SfizzVstEditor(void *controller);
    ~SfizzVstEditor();

    bool PLUGIN_API open(void* parent, const VSTGUI::PlatformType& platformType = VSTGUI::kDefaultNative) override;
    void PLUGIN_API close() override;

    SfizzVstController* getController() const
    {
        return static_cast<SfizzVstController*>(Vst::VSTGUIEditor::getController());
    }

    // IControlListener
    void valueChanged(CControl* ctl) override;
    void enterOrLeaveEdit(CControl* ctl, bool enter);
    void controlBeginEdit(CControl* ctl) override;
    void controlEndEdit(CControl* ctl) override;

    // SfizzVstController::StateListener
    void onStateChanged() override;

private:
    void chooseSfzFile();
    void loadSfzFile(const std::string& filePath);

    void createFrameContents();
    void updateStateDisplay();
    void setActivePanel(unsigned panelId);

    template <class Control>
    void adjustMinMaxToRangeParam(Control* c, Vst::ParamID id)
    {
        auto* p = static_cast<Vst::RangeParameter*>(getController()->getParameterObject(id));
        c->setMin(p->getMin());
        c->setMax(p->getMax());
    }

    enum {
        kPanelGeneral,
        // kPanelControls,
        kPanelSettings,
        kNumPanels,
    };

    unsigned _activePanel = 0;
    CViewContainer* _subPanels[kNumPanels] = {};

    enum {
        kTagLoadSfzFile,
        kTagSetVolume,
        kTagSetNumVoices,
        kTagSetOversampling,
        kTagSetPreloadSize,
        kTagFirstChangePanel,
        kTagLastChangePanel = kTagFirstChangePanel + kNumPanels - 1,
    };

    CBitmap _logo;
    CTextLabel* _fileLabel = nullptr;
    CSliderBase *_volumeSlider = nullptr;
    CSliderBase *_numVoicesSlider = nullptr;
    CSliderBase *_oversamplingSlider = nullptr;
    CSliderBase *_preloadSizeSlider = nullptr;
};
