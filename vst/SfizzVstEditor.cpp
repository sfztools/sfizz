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

///
class SimpleButton : public CControl {
public:
    explicit SimpleButton(const CRect& size, IControlListener* listener = nullptr, int32_t tag = -1, UTF8StringPtr title = nullptr)
        : CControl(size, listener, tag), _title(title ? title : "") {
    }

    void draw(CDrawContext *dc) override
    {
        CRect bounds = getViewSize();
        dc->setFrameColor(CColor(0xff, 0xff, 0xff));
        dc->drawRect(bounds, kDrawStroked);
        dc->drawString(_title.c_str(), bounds);
    }

    CMouseEventResult onMouseDown(CPoint& where, const CButtonState& buttons) override
    {
        if (!buttons.isLeftButton())
            return kMouseEventNotHandled;

        value = getMin();
        if (isDirty()) {
            valueChanged();
            invalid();
        }
        value = getMax();
        if (isDirty())
        {
            valueChanged();
            invalid();
        }

        return kMouseEventHandled;
    }

    CLASS_METHODS(SimpleButton, CControl)

private:
    std::string _title;
};

///
void SfizzVstEditor::createFrameContents()
{
    CFrame* frame = this->frame;
    CRect bounds = frame->getViewSize();

    frame->setBackgroundColor(CColor(0xff, 0xff, 0xff));

    CKickButton* sfizzButton = new CKickButton(bounds, this, kTagLoadSfzFile, &_logo);
    frame->addView(sfizzButton);

    CRect bottomRow = bounds;
    bottomRow.top = bottomRow.bottom - 30;

    CRect topRow = bounds;
    topRow.bottom = topRow.top + 30;

    CTextLabel* descLabel = new CTextLabel(
        bottomRow, "Paul Ferrand and the SFZ Tools work group");
    descLabel->setFontColor(CColor(0x00, 0x00, 0x00));
    descLabel->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
    frame->addView(descLabel);

    CRect fileBox = topRow;
    fileBox.right = fileBox.left + 400;
    CTextLabel* fileLabel = new CTextLabel(fileBox, "No file loaded");
    fileLabel->setFontColor(CColor(0x00, 0x00, 0x00));
    fileLabel->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
    // fileLabel->setHoriAlign(kLeftText);
    frame->addView(fileLabel);
    _fileLabel = fileLabel;

    // CTextLabel *label;
    // CRect rect;
    // CRect rect2;

    // rect = CRect(10.0, 10.0, 120.0, 30.0);
    // frame->addView(new SimpleButton(rect, this, kTagLoadSfzFile, "Load SFZ file"));

    // rect2 = CRect(150.0, 10.0, bounds.right - 10.0, 30.0);
    // frame->addView((label = new CTextLabel(rect2, "no file")));
    // label->setHoriAlign(kLeftText);
    // _fileLabel = label;
}

void SfizzVstEditor::updateStateDisplay()
{
    if (!frame)
        return;

    const SfizzVstState& state = static_cast<SfizzVstController*>(getController())->getSfizzState();

    if (_fileLabel)
        _fileLabel->setText(("File: " + state.sfzFile).c_str());
}
