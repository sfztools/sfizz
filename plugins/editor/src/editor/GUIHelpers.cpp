// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "GUIHelpers.h"
#define VSTGUI_MORE_THAN_4_10 ((VSTGUI_VERSION_MAJOR > 4) || (VSTGUI_VERSION_MAJOR == 4 && VSTGUI_VERSION_MINOR > 10))
#if VSTGUI_MORE_THAN_4_10
#include "utility/vstgui_before.h"
#include <vstgui/lib/events.h>
#include "utility/vstgui_after.h"
#endif

class SFrameDisabler::KeyAndMouseHook : public CBaseObject,
                                        public IKeyboardHook,
                                        public IMouseObserver {
public:
    void setEnabled(bool value) { enabled_ = value; }

protected:
    void onMouseEntered(CView*, CFrame*) override {}
    void onMouseExited(CView*, CFrame*) override {}
#if VSTGUI_MORE_THAN_4_10
    void onKeyboardEvent(KeyboardEvent& event, CFrame* frame) override;
    void onMouseEvent(MouseEvent& event, CFrame* frame) override;
#else
    int32_t onKeyDown(const VstKeyCode&, CFrame*) override { return enabled_ ? -1 : 1; }
    int32_t onKeyUp(const VstKeyCode&, CFrame*) override { return enabled_ ? -1 : 1; }
    CMouseEventResult onMouseMoved(CFrame*, const CPoint&, const CButtonState&) override { return enabled_ ? kMouseEventNotHandled : kMouseEventHandled; }
    CMouseEventResult onMouseDown(CFrame*, const CPoint&, const CButtonState&) override { return enabled_ ? kMouseEventNotHandled : kMouseEventHandled; }
#endif

private:
    bool enabled_ = true;
};

SFrameDisabler::SFrameDisabler(CFrame* frame)
    : frame_(frame), hook_(makeOwned<KeyAndMouseHook>())
{
    frame->registerKeyboardHook(hook_);
    frame->registerMouseObserver(hook_);

    delayedEnabler_ = makeOwned<CVSTGUITimer>(
        [this](CVSTGUITimer* t) { hook_->setEnabled(true); t->stop(); },
        1, false);
}

SFrameDisabler::~SFrameDisabler()
{
    frame_->unregisterKeyboardHook(hook_);
    frame_->unregisterMouseObserver(hook_);
}

void SFrameDisabler::enable()
{
    delayedEnabler_->start();
}

void SFrameDisabler::disable()
{
    hook_->setEnabled(false);
    delayedEnabler_->stop();
}

#if VSTGUI_MORE_THAN_4_10
void SFrameDisabler::KeyAndMouseHook::onKeyboardEvent(KeyboardEvent& event, CFrame*)
{
    if (!enabled_)
        event.consumed = true;
}

void SFrameDisabler::KeyAndMouseHook::onMouseEvent(MouseEvent& event, CFrame*)
{
    if (!enabled_)
        event.consumed = true;
}
#endif
