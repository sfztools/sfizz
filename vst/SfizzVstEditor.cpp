// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstEditor.h"
#include "SfizzVstState.h"
#include "GUIComponents.h"
#if !defined(__APPLE__) && !defined(_WIN32)
#include "X11RunLoop.h"
#endif

using namespace VSTGUI;

static ViewRect sfizzUiViewRect {0, 0, 482, 225};

SfizzVstEditor::SfizzVstEditor(void *controller)
    : VSTGUIEditor(controller, &sfizzUiViewRect),
      _logo("logo.png")
{
    getController()->addSfizzStateListener(this);
}

SfizzVstEditor::~SfizzVstEditor()
{
    getController()->removeSfizzStateListener(this);
}

bool PLUGIN_API SfizzVstEditor::open(void* parent, const VSTGUI::PlatformType& platformType)
{
    fprintf(stderr, "[sfizz] about to open view with parent %p\n", parent);

    CRect wsize(0, 0, sfizzUiViewRect.getWidth(), sfizzUiViewRect.getHeight());
    CFrame *frame = new CFrame(wsize, this);
    this->frame = frame;

    IPlatformFrameConfig* config = nullptr;

#if !defined(__APPLE__) && !defined(_WIN32)
    X11::FrameConfig x11config;
    if (!_runLoop)
        _runLoop = new RunLoop(plugFrame);
    x11config.runLoop = _runLoop;
    config = &x11config;
#endif

    createFrameContents();
    updateStateDisplay();

    if (!frame->open(parent, platformType, config)) {
        fprintf(stderr, "[sfizz] error opening frame\n");
        return false;
    }

    return true;
}

void PLUGIN_API SfizzVstEditor::close()
{
    CFrame *frame = this->frame;
    if (frame) {
        frame->removeAll();
        if (frame->getNbReference() != 1)
            frame->forget ();
        else {
            frame->close();
            this->frame = nullptr;
        }
    }
}

///
void SfizzVstEditor::valueChanged(CControl* ctl)
{
    int32_t tag = ctl->getTag();
    float value = ctl->getValue();
    float valueNorm = ctl->getValueNormalized();
    SfizzVstController* controller = getController();

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
        controller->setParamNormalized(kPidVolume, valueNorm);
        controller->performEdit(kPidVolume, valueNorm);
        updateVolumeLabel(value);
        break;

    case kTagSetNumVoices:
        controller->setParamNormalized(kPidNumVoices, valueNorm);
        controller->performEdit(kPidNumVoices, valueNorm);
        updateNumVoicesLabel(static_cast<int>(value));
        break;

    case kTagSetOversampling:
        controller->setParamNormalized(kPidOversampling, valueNorm);
        controller->performEdit(kPidOversampling, valueNorm);
        updateOversamplingLabel(static_cast<int>(value));
        break;

    case kTagSetPreloadSize:
        controller->setParamNormalized(kPidPreloadSize, valueNorm);
        controller->performEdit(kPidPreloadSize, valueNorm);
        updatePreloadSizeLabel(static_cast<int>(value));
        break;

    case kTagSetScalaRootKey:
        controller->setParamNormalized(kPidScalaRootKey, valueNorm);
        controller->performEdit(kPidScalaRootKey, valueNorm);
        updateScalaRootKeyLabel(static_cast<int>(value));
        break;

    case kTagSetTuningFrequency:
        controller->setParamNormalized(kPidTuningFrequency, valueNorm);
        controller->performEdit(kPidTuningFrequency, valueNorm);
        updateTuningFrequencyLabel(value);
        break;

    case kTagSetStretchedTuning:
        controller->setParamNormalized(kPidStretchedTuning, valueNorm);
        controller->performEdit(kPidStretchedTuning, valueNorm);
        updateStretchedTuningLabel(value);
        break;

    default:
        if (tag >= kTagFirstChangePanel && tag <= kTagLastChangePanel)
            setActivePanel(tag - kTagFirstChangePanel);
        break;
    }
}

void SfizzVstEditor::enterOrLeaveEdit(CControl* ctl, bool enter)
{
    int32_t tag = ctl->getTag();
    Vst::ParamID id;

    switch (tag) {
    case kTagSetVolume: id = kPidVolume; break;
    case kTagSetNumVoices: id = kPidNumVoices; break;
    case kTagSetOversampling: id = kPidOversampling; break;
    case kTagSetPreloadSize: id = kPidPreloadSize; break;
    case kTagSetScalaRootKey: id = kPidScalaRootKey; break;
    case kTagSetTuningFrequency: id = kPidTuningFrequency; break;
    case kTagSetStretchedTuning: id = kPidStretchedTuning; break;
    default: return;
    }

    SfizzVstController* controller = getController();
    if (enter)
        controller->beginEdit(id);
    else
        controller->endEdit(id);
}

void SfizzVstEditor::controlBeginEdit(CControl* ctl)
{
    enterOrLeaveEdit(ctl, true);
}

void SfizzVstEditor::controlEndEdit(CControl* ctl)
{
    enterOrLeaveEdit(ctl, false);
}

CMessageResult SfizzVstEditor::notify(CBaseObject* sender, const char* message)
{
    CMessageResult result = VSTGUIEditor::notify(sender, message);

    if (result != kMessageNotified)
        return result;

#if !defined(__APPLE__) && !defined(_WIN32)
    if (message == CVSTGUITimer::kMsgTimer) {
        SharedPointer<VSTGUI::RunLoop> runLoop = RunLoop::get();
        if (runLoop) {
            // note(jpc) I don't find a reliable way to check if the host
            //   notifier of X11 events is working. If there is, remove this and
            //   avoid polluting Linux hosts which implement the loop correctly.
            runLoop->processSomeEvents();

            runLoop->cleanupDeadHandlers();
        }
    }
#endif

    return result;
}

void SfizzVstEditor::onStateChanged()
{
    updateStateDisplay();
}

///
void SfizzVstEditor::chooseSfzFile()
{
    SharedPointer<CNewFileSelector> fs(CNewFileSelector::create(frame));

    fs->setTitle("Load SFZ file");
    fs->setDefaultExtension(CFileExtension("SFZ", "sfz"));

    if (fs->runModal()) {
        UTF8StringPtr file = fs->getSelectedFile(0);
        if (file)
            loadSfzFile(file);
    }
}

void SfizzVstEditor::loadSfzFile(const std::string& filePath)
{
    SfizzVstController* ctl = getController();

    Steinberg::OPtr<Vst::IMessage> msg { ctl->allocateMessage() };
    if (!msg) {
        fprintf(stderr, "[Sfizz] UI could not allocate message\n");
        return;
    }

    msg->setMessageID("LoadSfz");
    Vst::IAttributeList* attr = msg->getAttributes();
    attr->setBinary("File", filePath.data(), filePath.size());
    ctl->sendMessage(msg);

    updateSfzFileLabel(filePath);
}

void SfizzVstEditor::chooseScalaFile()
{
    SharedPointer<CNewFileSelector> fs(CNewFileSelector::create(frame));

    fs->setTitle("Load Scala file");
    fs->setDefaultExtension(CFileExtension("SCL", "scl"));

    if (fs->runModal()) {
        UTF8StringPtr file = fs->getSelectedFile(0);
        if (file)
            loadScalaFile(file);
    }
}

void SfizzVstEditor::loadScalaFile(const std::string& filePath)
{
    SfizzVstController* ctl = getController();

    Steinberg::OPtr<Vst::IMessage> msg { ctl->allocateMessage() };
    if (!msg) {
        fprintf(stderr, "[Sfizz] UI could not allocate message\n");
        return;
    }

    msg->setMessageID("LoadScala");
    Vst::IAttributeList* attr = msg->getAttributes();
    attr->setBinary("File", filePath.data(), filePath.size());
    ctl->sendMessage(msg);

    updateScalaFileLabel(filePath);
}

void SfizzVstEditor::createFrameContents()
{
    SfizzVstController* controller = getController();
    const SfizzUiState& uiState = controller->getSfizzUiState();

    CFrame* frame = this->frame;
    CRect bounds = frame->getViewSize();

    frame->setBackgroundColor(CColor(0xff, 0xff, 0xff));

    CRect bottomRow = bounds;
    bottomRow.top = bottomRow.bottom - 30;

    CRect topRow = bounds;
    topRow.bottom = topRow.top + 30;

    CViewContainer* panel;
    _activePanel = std::max(0, std::min(kNumPanels - 1, static_cast<int>(uiState.activePanel)));

    CRect topLeftLabelBox = topRow;
    topLeftLabelBox.right -= 20 * kNumPanels;

    // general panel
    {
        panel = new CViewContainer(bounds);
        frame->addView(panel);
        panel->setTransparency(true);

        CKickButton* sfizzButton = new CKickButton(bounds, this, kTagLoadSfzFile, &_logo);
        panel->addView(sfizzButton);

        CTextLabel* topLeftLabel = new CTextLabel(topLeftLabelBox, "No file loaded");
        topLeftLabel->setFontColor(CColor(0x00, 0x00, 0x00));
        topLeftLabel->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        panel->addView(topLeftLabel);
        _sfzFileLabel = topLeftLabel;

        _subPanels[kPanelGeneral] = panel;
    }

    // settings panel
    {
        panel = new CViewContainer(bounds);
        frame->addView(panel);
        panel->setTransparency(true);

        CTextLabel* topLeftLabel = new CTextLabel(topLeftLabelBox, "Settings");
        topLeftLabel->setFontColor(CColor(0x00, 0x00, 0x00));
        topLeftLabel->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        panel->addView(topLeftLabel);

        CRect row = topRow;
        row.top += 45.0;
        row.bottom += 45.0;
        row.left += 100.0;
        row.right -= 100.0;

        CCoord interRow = 35.0;

        auto leftSide = [&row]() -> CRect {
            CRect div = row;
            div.right = 0.5 * (div.left + div.right);
            return div;
        };

        auto rightSide = [&row]() -> CRect {
            CRect div = row;
            div.left = 0.5 * (div.left + div.right);
            return div;
        };

        auto labelArea = [&topRow, &row]() -> CRect {
            CRect div = row;
            div.right = topRow.right - 10.0;
            div.left = div.right - 100.0 + 20.0;
            return div;
        };

        CTextLabel* label;
        SimpleSlider* slider;

        label = new CTextLabel(leftSide(), "Volume");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(rightSide(), this, kTagSetVolume);
        panel->addView(slider);
        adjustMinMaxToRangeParam(slider, kPidVolume);
        _volumeSlider = slider;
        label = new CTextLabel(labelArea(), "");
        _volumeLabel = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(leftSide(), "Polyphony");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(rightSide(), this, kTagSetNumVoices);
        panel->addView(slider);
        adjustMinMaxToRangeParam(slider, kPidNumVoices);
        _numVoicesSlider = slider;
        label = new CTextLabel(labelArea(), "");
        _numVoicesLabel = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(leftSide(), "Oversampling");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(rightSide(), this, kTagSetOversampling);
        panel->addView(slider);
        adjustMinMaxToRangeParam(slider, kPidOversampling);
        _oversamplingSlider = slider;
        label = new CTextLabel(labelArea(), "");
        _oversamplingLabel = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(leftSide(), "Preload size");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(rightSide(), this, kTagSetPreloadSize);
        panel->addView(slider);
        adjustMinMaxToRangeParam(slider, kPidPreloadSize);
        _preloadSizeSlider = slider;
        label = new CTextLabel(labelArea(), "");
        _preloadSizeLabel = label;
        panel->addView(label);

        // row.top += interRow;
        // row.bottom += interRow;

        // label = new CTextLabel(leftSide(), "Freewheel");
        // label->setFontColor(CColor(0x00, 0x00, 0x00));
        // label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        // label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        // label->setHoriAlign(kLeftText);
        // panel->addView(label);
        // slider = new SimpleSlider(rightSide(), this, kTag);
        // panel->addView(slider);
        // adjustMinMaxToRangeParam(slider, kPid);
        // _aSlider = slider;

        _subPanels[kPanelSettings] = panel;
    }

    // tuning panel
    {
        panel = new CViewContainer(bounds);
        frame->addView(panel);
        panel->setTransparency(true);

        CTextLabel* topLeftLabel = new CTextLabel(topLeftLabelBox, "Tuning");
        topLeftLabel->setFontColor(CColor(0x00, 0x00, 0x00));
        topLeftLabel->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        panel->addView(topLeftLabel);

        CRect row = topRow;
        row.top += 45.0;
        row.bottom += 45.0;
        row.left += 100.0;
        row.right -= 100.0;

        CCoord interRow = 35.0;

        auto leftSide = [&row]() -> CRect {
            CRect div = row;
            div.right = 0.5 * (div.left + div.right);
            return div;
        };

        auto rightSide = [&row]() -> CRect {
            CRect div = row;
            div.left = 0.5 * (div.left + div.right);
            return div;
        };

        auto labelArea = [&topRow, &row]() -> CRect {
            CRect div = row;
            div.right = topRow.right - 10.0;
            div.left = div.right - 100.0 + 20.0;
            return div;
        };

        CTextLabel* label;
        SimpleSlider* slider;
        CTextButton* textbutton;

        label = new CTextLabel(leftSide(), "Scala file");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        textbutton = new CTextButton(rightSide(), this, kTagLoadScalaFile, "Choose");
        panel->addView(textbutton);
        label = new CTextLabel(labelArea(), "");
        _scalaFileLabel = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(leftSide(), "Scala root key");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(rightSide(), this, kTagSetScalaRootKey);
        panel->addView(slider);
        adjustMinMaxToRangeParam(slider, kPidScalaRootKey);
        _scalaRootKeySlider = slider;
        label = new CTextLabel(labelArea(), "");
        _scalaRootKeyLabel = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(leftSide(), "Tuning frequency");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(rightSide(), this, kTagSetTuningFrequency);
        panel->addView(slider);
        adjustMinMaxToRangeParam(slider, kPidTuningFrequency);
        _tuningFrequencySlider = slider;
        label = new CTextLabel(labelArea(), "");
        _tuningFrequencyLabel = label;
        panel->addView(label);

        row.top += interRow;
        row.bottom += interRow;

        label = new CTextLabel(leftSide(), "Stretched tuning");
        label->setFontColor(CColor(0x00, 0x00, 0x00));
        label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        label->setHoriAlign(kLeftText);
        panel->addView(label);
        slider = new SimpleSlider(rightSide(), this, kTagSetStretchedTuning);
        panel->addView(slider);
        adjustMinMaxToRangeParam(slider, kPidStretchedTuning);
        _stretchedTuningSlider = slider;
        label = new CTextLabel(labelArea(), "");
        _stretchedTuningLabel = label;
        panel->addView(label);

        _subPanels[kPanelTuning] = panel;
    }

    // all panels
    for (unsigned currentPanel = 0; currentPanel < kNumPanels; ++currentPanel) {
        panel = _subPanels[currentPanel];

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
            default: text = "?"; break;
            }

            CTextButton* changePanelButton = new CTextButton(btnRect, this, kTagFirstChangePanel + i, text);
            panel->addView(changePanelButton);

            changePanelButton->setRoundRadius(0.0);
        }

        panel->setVisible(currentPanel == _activePanel);
    }
}

void SfizzVstEditor::updateStateDisplay()
{
    if (!frame)
        return;

    SfizzVstController* controller = getController();
    const SfizzVstState& state = controller->getSfizzState();
    const SfizzUiState& uiState = controller->getSfizzUiState();

    updateSfzFileLabel(state.sfzFile);
    if (_volumeSlider)
        _volumeSlider->setValue(state.volume);
    updateVolumeLabel(state.volume);
    if (_numVoicesSlider)
        _numVoicesSlider->setValue(state.numVoices);
    updateNumVoicesLabel(state.numVoices);
    if (_oversamplingSlider)
        _oversamplingSlider->setValue(state.oversamplingLog2);
    updateOversamplingLabel(state.oversamplingLog2);
    if (_preloadSizeSlider)
        _preloadSizeSlider->setValue(state.preloadSize);
    updatePreloadSizeLabel(state.preloadSize);
    updateScalaFileLabel(state.scalaFile);
    if (_scalaRootKeySlider)
        _scalaRootKeySlider->setValue(state.scalaRootKey);
    updateScalaRootKeyLabel(state.scalaRootKey);
    if (_tuningFrequencySlider)
        _tuningFrequencySlider->setValue(state.tuningFrequency);
    updateTuningFrequencyLabel(state.tuningFrequency);
    if (_stretchedTuningSlider)
        _stretchedTuningSlider->setValue(state.stretchedTuning);
    updateStretchedTuningLabel(state.stretchedTuning);

    setActivePanel(uiState.activePanel);
}

void SfizzVstEditor::updateSfzFileLabel(const std::string& filePath)
{
    updateLabelWithFileName(_sfzFileLabel, filePath);
}

void SfizzVstEditor::updateScalaFileLabel(const std::string& filePath)
{
    updateLabelWithFileName(_scalaFileLabel, filePath);
}

void SfizzVstEditor::updateLabelWithFileName(CTextLabel* label, const std::string& filePath)
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

void SfizzVstEditor::updateVolumeLabel(float volume)
{
    CTextLabel* label = _volumeLabel;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%.1f dB", volume);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void SfizzVstEditor::updateNumVoicesLabel(int numVoices)
{
    CTextLabel* label = _numVoicesLabel;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%d", numVoices);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void SfizzVstEditor::updateOversamplingLabel(int oversamplingLog2)
{
    CTextLabel* label = _oversamplingLabel;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%dx", 1 << oversamplingLog2);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void SfizzVstEditor::updatePreloadSizeLabel(int preloadSize)
{
    CTextLabel* label = _preloadSizeLabel;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%.1f kB", preloadSize * (1.0 / 1024));
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void SfizzVstEditor::updateScalaRootKeyLabel(int rootKey)
{
    CTextLabel* label = _scalaRootKeyLabel;
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

void SfizzVstEditor::updateTuningFrequencyLabel(float tuningFrequency)
{
    CTextLabel* label = _tuningFrequencyLabel;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%.1f", tuningFrequency);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void SfizzVstEditor::updateStretchedTuningLabel(float stretchedTuning)
{
    CTextLabel* label = _stretchedTuningLabel;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%.3f", stretchedTuning);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}


void SfizzVstEditor::setActivePanel(unsigned panelId)
{
    panelId = std::max(0, std::min(kNumPanels - 1, static_cast<int>(panelId)));

    getController()->getSfizzUiState().activePanel = panelId;

    if (_activePanel != panelId) {
        if (frame)
            _subPanels[_activePanel]->setVisible(false);

        _activePanel = panelId;

        if (frame)
            _subPanels[panelId]->setVisible(true);
    }
}
