// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "GUIHelpers.h"
#include "utility/vstgui_before.h"
#include <vstgui/lib/events.h>
#include "utility/vstgui_after.h"

class SFrameDisabler::KeyAndMouseHook : public CBaseObject,
                                        public IKeyboardHook,
                                        public IMouseObserver {
public:
    void setEnabled(bool value) { enabled_ = value; }

protected:
    void onKeyboardEvent(KeyboardEvent& event, CFrame* frame) override;
    void onMouseEntered(CView* view, CFrame* frame) override {}
    void onMouseExited(CView* view, CFrame* frame) override {}
    void onMouseEvent(MouseEvent& event, CFrame* frame) override;

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

void SFrameDisabler::KeyAndMouseHook::onKeyboardEvent(KeyboardEvent& event, CFrame* frame)
{
    if (!enabled_)
        event.consumed = true;
}

void SFrameDisabler::KeyAndMouseHook::onMouseEvent(MouseEvent& event, CFrame* frame)
{
    if (!enabled_)
        event.consumed = true;
}
