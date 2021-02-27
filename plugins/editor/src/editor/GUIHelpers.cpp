// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "GUIHelpers.h"

class SFrameDisabler::KeyAndMouseHook : public CBaseObject,
                                        public IKeyboardHook,
                                        public IMouseObserver {
public:
    void setEnabled(bool value) { enabled_ = value; }

protected:
    int32_t onKeyDown(const VstKeyCode&, CFrame*) { return enabled_ ? -1 : 1; }
    int32_t onKeyUp(const VstKeyCode&, CFrame*) { return enabled_ ? -1 : 1; }
    void onMouseEntered(CView*, CFrame*) {}
    void onMouseExited(CView*, CFrame*) {}
    CMouseEventResult onMouseMoved(CFrame*, const CPoint&, const CButtonState&) { return enabled_ ? kMouseEventNotHandled : kMouseEventHandled; }
    CMouseEventResult onMouseDown(CFrame*, const CPoint&, const CButtonState&) { return enabled_ ? kMouseEventNotHandled : kMouseEventHandled; }

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
