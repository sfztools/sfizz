// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstEditor.h"
#include "SfizzVstState.h"
#include "GUIComponents.h"
#if !defined(__APPLE__) && !defined(_WIN32)
#include "x11runloop.h"
#endif

using namespace VSTGUI;

SfizzVstEditor::SfizzVstEditor(void *controller)
    : VSTGUIEditor(controller),
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
    CRect wsize(0, 0, _logo.getWidth(), _logo.getHeight());
    CFrame *frame = new CFrame(wsize, this);
    this->frame = frame;

    IPlatformFrameConfig* config = nullptr;

#if !defined(__APPLE__) && !defined(_WIN32)
    X11::FrameConfig x11config;
    x11config.runLoop = VSTGUI::owned(new RunLoop(plugFrame));
    config = &x11config;
#endif

    createFrameContents();
    updateStateDisplay();

    frame->open(parent, platformType, config);
    return true;
}

void PLUGIN_API SfizzVstEditor::close()
{
    CFrame *frame = this->frame;
    if (frame) {
        frame->forget();
        this->frame = nullptr;
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
    attr->setString("File", Steinberg::String(filePath.c_str()).text());
    ctl->sendMessage(msg);
    msg->release();

    if (_fileLabel)
        _fileLabel->setText(("File: " + filePath).c_str());
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
        row.top += 200.0;
        row.bottom += 200.0;
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
        adjustMinMaxToRangeParam(slider, kPidVolume);
        panel->addView(slider);
        _volumeSlider = slider;

        // row.top += interRow;
        // row.bottom += interRow;

        // label = new CTextLabel(leftSide(), "Polyphony");
        // label->setFontColor(CColor(0x00, 0x00, 0x00));
        // label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        // label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        // label->setHoriAlign(kLeftText);
        // panel->addView(label);
        // slider = new SimpleSlider(rightSide(), this, -1);
        // panel->addView(slider);

        // row.top += interRow;
        // row.bottom += interRow;

        // label = new CTextLabel(leftSide(), "Oversampling");
        // label->setFontColor(CColor(0x00, 0x00, 0x00));
        // label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        // label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        // label->setHoriAlign(kLeftText);
        // panel->addView(label);
        // slider = new SimpleSlider(rightSide(), this, -1);
        // panel->addView(slider);

        // row.top += interRow;
        // row.bottom += interRow;

        // label = new CTextLabel(leftSide(), "Preload size");
        // label->setFontColor(CColor(0x00, 0x00, 0x00));
        // label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        // label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        // label->setHoriAlign(kLeftText);
        // panel->addView(label);
        // slider = new SimpleSlider(rightSide(), this, -1);
        // panel->addView(slider);

        // row.top += interRow;
        // row.bottom += interRow;

        // label = new CTextLabel(leftSide(), "Freewheel");
        // label->setFontColor(CColor(0x00, 0x00, 0x00));
        // label->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        // label->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
        // label->setHoriAlign(kLeftText);
        // panel->addView(label);
        // slider = new SimpleSlider(rightSide(), this, -1);
        // panel->addView(slider);

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
            btnRect.left = topRow.right - (kNumPanels - i) * 20;
            btnRect.right = btnRect.left + 20;

            const char *text;
            switch (i) {
            case kPanelGeneral: text = "G"; break;
            case kPanelSettings: text = "S"; break;
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

    if (_fileLabel)
        _fileLabel->setText(("File: " + state.sfzFile).c_str());
    if (_volumeSlider)
        _volumeSlider->setValue(state.volume);

    setActivePanel(uiState.activePanel);
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
