// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstController.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include <memory>
class Editor;

using namespace Steinberg;

class SfizzVstEditor : public Vst::EditorView,
                       public SfizzVstController::StateListener {
public:
    explicit SfizzVstEditor(Vst::EditController* controller);
    ~SfizzVstEditor();

    void onStateChanged() override;

    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override;
    tresult PLUGIN_API attached(void* parent, FIDString type) override;
    tresult PLUGIN_API removed() override;

private:
    std::unique_ptr<Editor> editor_;

#if !defined(__APPLE__) && !defined(_WIN32)
    class IdleTimerHandler;
    friend class IdleTimerHandler;
    std::unique_ptr<IdleTimerHandler> idleTimerHandler_;
#endif
};
