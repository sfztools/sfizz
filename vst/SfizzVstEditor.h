// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstController.h"
#include "editor/EditorController.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include <memory>
class Editor;
class NativeIdleRunner;

using namespace Steinberg;

class SfizzVstEditor : public Vst::EditorView,
                       public SfizzVstController::StateListener,
                       public EditorController {
public:
    explicit SfizzVstEditor(SfizzVstController* controller);
    ~SfizzVstEditor();

    void onStateChanged() override;

    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override;
    tresult PLUGIN_API attached(void* parent, FIDString type) override;
    tresult PLUGIN_API removed() override;

private:
    void updateStateDisplay();

protected:
    // EditorController
    void uiSendNumber(EditId id, float v) override;
    void uiSendString(EditId id, absl::string_view v) override;
    void uiBeginSend(EditId id) override;
    void uiEndSend(EditId id) override;
    void uiSendMIDI(const uint8_t* msg, uint32_t len) override;

private:
    void uiTouch(EditId id, bool t);

private:
    std::unique_ptr<Editor> editor_;

#if !defined(__APPLE__) && !defined(_WIN32)
    class IdleTimerHandler;
    friend class IdleTimerHandler;
    std::unique_ptr<IdleTimerHandler> idleTimerHandler_;
#else
    std::unique_ptr<NativeIdleRunner> idleRunner_;
#endif
};
