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

    case kTagSetVolume:
        controller->setParamNormalized(kPidVolume, valueNorm);
        controller->performEdit(kPidVolume, valueNorm);
        break;

    case kTagSetNumVoices:
        controller->setParamNormalized(kPidNumVoices, valueNorm);
        controller->performEdit(kPidNumVoices, valueNorm);
        break;

    case kTagSetOversampling:
        controller->setParamNormalized(kPidOversampling, valueNorm);
        controller->performEdit(kPidOversampling, valueNorm);
        break;

    case kTagSetPreloadSize:
        controller->setParamNormalized(kPidPreloadSize, valueNorm);
        controller->performEdit(kPidPreloadSize, valueNorm);
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

    Vst::IMessage *msg = ctl->allocateMessage();
    if (!msg) {
        fprintf(stderr, "[Sfizz] UI could not allocate message\n");
        return;
    }

    msg->setMessageID("LoadSfz");
    Vst::IAttributeList* attr = msg->getAttributes();
    attr->setBinary("File", filePath.data(), filePath.size());
    ctl->sendMessage(msg);
    msg->release();

    updateFileLabel(filePath);
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
        _fileLabel = topLeftLabel;

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

    updateFileLabel(state.sfzFile);
    if (_volumeSlider)
        _volumeSlider->setValue(state.volume);
    if (_numVoicesSlider)
        _numVoicesSlider->setValue(state.numVoices);
    if (_oversamplingSlider)
        _oversamplingSlider->setValue(state.oversamplingLog2);
    if (_preloadSizeSlider)
        _preloadSizeSlider->setValue(state.preloadSize);

    setActivePanel(uiState.activePanel);
}

void SfizzVstEditor::updateFileLabel(const std::string& filePath)
{
    if (_fileLabel) {
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
        _fileLabel->setText(fileName.c_str());
    }
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
