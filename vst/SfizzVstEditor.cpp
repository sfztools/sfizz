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
SfizzVstEditor::SfizzVstEditor(SfizzVstController* controller)
    : EditorView(controller, &kEditorViewRect)
#if !defined(__APPLE__) && !defined(_WIN32)
    , idleTimerHandler_(new IdleTimerHandler(*this))
#else
    , idleRunner_(new NativeIdleRunner)
#endif
{
    Res::initializeRootPathFromCurrentModule("../Resources");

    controller->addSfizzStateListener(this);
}

SfizzVstEditor::~SfizzVstEditor()
{
    auto* controller = static_cast<SfizzVstController*>(getController());
    controller->removeSfizzStateListener(this);
}

void SfizzVstEditor::onStateChanged()
{
    updateStateDisplay();
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

    Editor* editor = new Editor(*this);
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

    updateStateDisplay();

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

///
void SfizzVstEditor::updateStateDisplay()
{
    if (!editor_)
        return;

    auto* controller = static_cast<SfizzVstController*>(getController());
    const SfizzVstState& state = controller->getSfizzState();
    const SfizzUiState& uiState = controller->getSfizzUiState();

    uiReceiveString(EditId::SfzFile, state.sfzFile);
    uiReceiveNumber(EditId::Volume, state.volume);
    uiReceiveNumber(EditId::Polyphony, state.numVoices);
    uiReceiveNumber(EditId::Oversampling, 1 << state.oversamplingLog2);
    uiReceiveNumber(EditId::PreloadSize, state.preloadSize);

    // TODO(jpc) when implemented: ScalaFile, ScalaRootKey, TuningFrequency, StretchTuning
}

///
void SfizzVstEditor::uiSendNumber(EditId id, float v)
{
    auto* controller = static_cast<SfizzVstController*>(getController());

    auto normalizeAndSend = [controller](int pid, float v, const SfizzParameterRange& range) {
        float vn = range.normalize(v);
        controller->setParamNormalized(pid, v);
        controller->performEdit(pid, v);
    };

    switch (id) {
    case EditId::Volume:
        normalizeAndSend(kPidVolume, v, kParamVolumeRange);
        break;
    case EditId::Polyphony:
        normalizeAndSend(kPidNumVoices, v, kParamNumVoicesRange);
        break;
    case EditId::Oversampling:
        {
            int32 factor = std::max(1, static_cast<int32>(v));

            // convert UI value using integer log2
            int32 log2Factor = 0;
            for (int32 f = factor; f > 1; f /= 2)
                ++log2Factor;

            normalizeAndSend(kPidOversampling, log2Factor, kParamOversamplingRange);
        }
        break;
    case EditId::PreloadSize:
        normalizeAndSend(kPidPreloadSize, v, kParamPreloadSizeRange);
        break;
    default:
        break;
    }
}

void SfizzVstEditor::uiSendString(EditId id, absl::string_view v)
{
    auto* controller = static_cast<SfizzVstController*>(getController());

    switch (id) {
    case EditId::SfzFile:
        {
            Steinberg::OPtr<Vst::IMessage> msg { controller->allocateMessage() };
            if (!msg) {
                fprintf(stderr, "[Sfizz] UI could not allocate message\n");
                break;
            }
            msg->setMessageID("LoadSfz");
            Vst::IAttributeList* attr = msg->getAttributes();
            attr->setBinary("File", v.data(), v.size());
            controller->sendMessage(msg);
        }
        break;
    default:
        break;
    }
}

void SfizzVstEditor::uiBeginSend(EditId id)
{
    uiTouch(id, true);
}

void SfizzVstEditor::uiEndSend(EditId id)
{
    uiTouch(id, false);
}

void SfizzVstEditor::uiTouch(EditId id, bool t)
{
    auto* controller = static_cast<SfizzVstController*>(getController());

    tresult (Vst::EditController::*touch)(Vst::ParamID) = t ?
        &Vst::EditController::beginEdit : &Vst::EditController::endEdit;

    switch (id) {
    case EditId::Volume:
        (controller->*touch)(kPidVolume);
        break;
    case EditId::Polyphony:
        (controller->*touch)(kPidNumVoices);
        break;
    case EditId::Oversampling:
        (controller->*touch)(kPidOversampling);
        break;
    case EditId::PreloadSize:
        (controller->*touch)(kPidPreloadSize);
        break;
    default:
        break;
    }
}

void SfizzVstEditor::uiSendMIDI(const uint8_t* msg, uint32_t len)
{
}
