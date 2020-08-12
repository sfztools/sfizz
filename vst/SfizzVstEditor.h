// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstController.h"
#include "public.sdk/source/vst/vstguieditor.h"
#if !defined(__APPLE__) && !defined(_WIN32)
namespace VSTGUI { class RunLoop; }
#endif

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

    // VSTGUIEditor
    CMessageResult notify(CBaseObject* sender, const char* message) override;

    // SfizzVstController::StateListener
    void onStateChanged() override;

private:
    void chooseSfzFile();
    void loadSfzFile(const std::string& filePath);

    void chooseScalaFile();
    void loadScalaFile(const std::string& filePath);

    void createFrameContents();
    void updateStateDisplay();
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
        kPanelTuning,
        kPanelInfo,
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
        kTagLoadScalaFile,
        kTagSetScalaRootKey,
        kTagSetTuningFrequency,
        kTagSetStretchedTuning,
        kTagFirstChangePanel,
        kTagLastChangePanel = kTagFirstChangePanel + kNumPanels - 1,
    };

    CBitmap _logo;
    CTextLabel* _sfzFileLabel = nullptr;
    CTextLabel* _scalaFileLabel = nullptr;
    CSliderBase *_volumeSlider = nullptr;
    CTextLabel* _volumeLabel = nullptr;
    CSliderBase *_numVoicesSlider = nullptr;
    CTextLabel* _numVoicesLabel = nullptr;
    CSliderBase *_oversamplingSlider = nullptr;
    CTextLabel* _oversamplingLabel = nullptr;
    CSliderBase *_preloadSizeSlider = nullptr;
    CTextLabel* _preloadSizeLabel = nullptr;
    CSliderBase *_scalaRootKeySlider = nullptr;
    CTextLabel* _scalaRootKeyLabel = nullptr;
    CSliderBase *_tuningFrequencySlider = nullptr;
    CTextLabel* _tuningFrequencyLabel = nullptr;
    CSliderBase *_stretchedTuningSlider = nullptr;
    CTextLabel* _stretchedTuningLabel = nullptr;

    CTextLabel* _infoCurvesLabel = nullptr;
    CTextLabel* _infoMastersLabel = nullptr;
    CTextLabel* _infoGroupsLabel = nullptr;
    CTextLabel* _infoRegionsLabel = nullptr;
    CTextLabel* _infoSamplesLabel = nullptr;
    CTextLabel* _infoVoicesLabel = nullptr;

#if !defined(__APPLE__) && !defined(_WIN32)
    SharedPointer<RunLoop> _runLoop;
#endif
};
