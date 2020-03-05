// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstEditor.h"
#include "SfizzVstState.h"
#if !defined(__APPLE__) && !defined(_WIN32)
#include "x11runloop.h"
#endif

using namespace VSTGUI;

SfizzVstEditor::SfizzVstEditor(void *controller)
    : VSTGUIEditor(controller),
      _logo("logo.png")
{
    static_cast<SfizzVstController*>(getController())->addStateListener(this);
}

SfizzVstEditor::~SfizzVstEditor()
{
    static_cast<SfizzVstController*>(getController())->removeStateListener(this);
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

    switch (tag) {
    case kTagLoadSfzFile:
        if (value != 1)
            break;

        Call::later([this]() { chooseSfzFile(); });
        break;

    default:
        if (tag >= kTagFirstChangePanel && tag <= kTagLastChangePanel)
            setActivePanel(tag - kTagFirstChangePanel);
        break;
    }
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
    Vst::EditController* ctl = getController();


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
    CFrame* frame = this->frame;
    CRect bounds = frame->getViewSize();

    frame->setBackgroundColor(CColor(0xff, 0xff, 0xff));

    CRect bottomRow = bounds;
    bottomRow.top = bottomRow.bottom - 30;

    CRect topRow = bounds;
    topRow.bottom = topRow.top + 30;

    CViewContainer* panel;
    _activePanel = kPanelGeneral;

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

    const SfizzVstState& state = static_cast<SfizzVstController*>(getController())->getSfizzState();

    if (_fileLabel)
        _fileLabel->setText(("File: " + state.sfzFile).c_str());
}

void SfizzVstEditor::setActivePanel(unsigned panelId)
{
    if (_activePanel != panelId) {
        _subPanels[_activePanel]->setVisible(false);
        _subPanels[panelId]->setVisible(true);
        _activePanel = panelId;
    }
}
