// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstEditor.h"
#include "SfizzVstState.h"
#include "editor/Editor.h"
#include "editor/Res.h"
#include <cstring>
#if !defined(__APPLE__) && !defined(_WIN32)
/**/
#else
#include "NativeIdleRunner.h"
#endif

static /*const*/ ViewRect kEditorViewRect { 0, 0, Editor::fixedWidth, Editor::fixedHeight };

static constexpr unsigned kIdleTimerInterval = 25;

///
#if !defined(__APPLE__) && !defined(_WIN32)
class SfizzVstEditor::IdleTimerHandler : public Linux::ITimerHandler, public Steinberg::FObject {
public:
    explicit IdleTimerHandler(SfizzVstEditor& editor)
        : editor_(editor)
    {
    }

    void PLUGIN_API onTimer() override
    {
        if (Editor* ui = editor_.editor_.get())
            ui->processEvents();
    }

    DELEGATE_REFCOUNT(Steinberg::FObject)
    DEFINE_INTERFACES
    DEF_INTERFACE(Steinberg::Linux::ITimerHandler)
    END_DEFINE_INTERFACES(Steinberg::FObject)

private:
    SfizzVstEditor& editor_;
};
#endif

///
SfizzVstEditor::SfizzVstEditor(Vst::EditController* controller)
    : EditorView(controller, &kEditorViewRect)
#if !defined(__APPLE__) && !defined(_WIN32)
    , idleTimerHandler_(new IdleTimerHandler(*this))
#else
    , idleRunner_(new NativeIdleRunner)
#endif
{
    Res::initializeRootPathFromCurrentModule("../Resources");
}

SfizzVstEditor::~SfizzVstEditor()
{
}

void SfizzVstEditor::onStateChanged()
{
    //TODO
}

tresult PLUGIN_API SfizzVstEditor::isPlatformTypeSupported(FIDString type)
{
#if SMTG_OS_WINDOWS
    if (strcmp(type, kPlatformTypeHWND) == 0)
        return kResultTrue;
#endif

#if SMTG_OS_MACOS
    if (strcmp(type, kPlatformTypeNSView) == 0)
        return kResultTrue;
#endif

#if SMTG_OS_LINUX
    if (strcmp(type, kPlatformTypeX11EmbedWindowID) == 0)
        return kResultTrue;
#endif

    return kInvalidArgument;
}

tresult PLUGIN_API SfizzVstEditor::attached(void* parent, FIDString type)
{
    if (isPlatformTypeSupported(type) != kResultTrue)
        return kResultFalse;

    Editor* editor = new Editor;
    editor_.reset(editor);

    if (!editor->open(parent)) {
        editor_.reset();
        return kResultFalse;
    }

#if !defined(__APPLE__) && !defined(_WIN32)
    Linux::IRunLoop* runLoop = nullptr;
    if (plugFrame->queryInterface(Linux::IRunLoop_iid, reinterpret_cast<void**>(&runLoop)) == kResultOk)
        runLoop->registerTimer(idleTimerHandler_.get(), kIdleTimerInterval);
#else
    auto editorIdleCallback = [](void* cbdata) {
        return reinterpret_cast<Editor*>(cbdata)->processEvents();
    };
    idleRunner_->start(kIdleTimerInterval * 1e-3, editorIdleCallback, editor);
#endif

    EditorView::attached(parent, type);
    return kResultOk;
}

tresult PLUGIN_API SfizzVstEditor::removed()
{
    if (!editor_)
        return kResultOk;

#if !defined(__APPLE__) && !defined(_WIN32)
    Linux::IRunLoop* runLoop = nullptr;
    if (plugFrame->queryInterface(Linux::IRunLoop_iid, reinterpret_cast<void**>(&runLoop)) == kResultOk)
        runLoop->unregisterTimer(idleTimerHandler_.get());
#else
    idleRunner_->stop();
#endif

    editor_->close();
    editor_.reset();

    EditorView::removed();
    return kResultOk;
}
