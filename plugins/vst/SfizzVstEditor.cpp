// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstEditor.h"
#include "SfizzVstState.h"
#include "SfizzVstParameters.h"
#include "SfizzVstUpdates.h"
#include "SfizzFileScan.h"
#include "editor/Editor.h"
#include "editor/EditIds.h"
#include "plugin/InstrumentDescription.h"
#include "IdleUpdateHandler.h"
#if !defined(__APPLE__) && !defined(_WIN32)
#include "X11RunLoop.h"
#endif

using namespace VSTGUI;

static ViewRect sfizzUiViewRect { 0, 0, Editor::viewWidth, Editor::viewHeight };

enum {
    kOscTempSize = 8192,
    kOscQueueSize = 65536,
    kNoteEventQueueSize = 8192,
};

SfizzVstEditor::SfizzVstEditor(
    SfizzVstController* controller,
    absl::Span<FObject*> continuousUpdates,
    absl::Span<FObject*> triggerUpdates)
    : VSTGUIEditor(controller, &sfizzUiViewRect),
      oscTemp_(new uint8_t[kOscTempSize]),
      continuousUpdates_(continuousUpdates.begin(), continuousUpdates.end()),
      triggerUpdates_(triggerUpdates.begin(), triggerUpdates.end())
{
}

SfizzVstEditor::~SfizzVstEditor()
{
}

bool PLUGIN_API SfizzVstEditor::open(void* parent, const VSTGUI::PlatformType& platformType)
{
    fprintf(stderr, "[sfizz] about to open view with parent %p\n", parent);

    CRect wsize(0, 0, sfizzUiViewRect.getWidth(), sfizzUiViewRect.getHeight());
    CFrame *frame = new CFrame(wsize, this);
    this->frame = frame;

    IPlatformFrameConfig* config = nullptr;

#if !defined(__APPLE__) && !defined(_WIN32)
    X11::FrameConfig x11config;
    if (!_runLoop)
        _runLoop = new RunLoop(plugFrame);
    x11config.runLoop = _runLoop;
    config = &x11config;
#endif

    Editor* editor = editor_.get();
    if (!editor) {
        editor = new Editor(*this);
        editor_.reset(editor);
    }

    {
        std::lock_guard<std::mutex> lock(oscQueueMutex_);
        OscByteVec* queue = new OscByteVec;
        oscQueue_.reset(queue);
        queue->reserve(kOscQueueSize);
    }
    {
        std::lock_guard<std::mutex> lock(noteEventQueueMutex_);
        NoteEventsVec* queue = new NoteEventsVec;
        noteEventQueue_.reset(queue);
        queue->reserve(kNoteEventQueueSize);
    }

    if (!frame->open(parent, platformType, config)) {
        fprintf(stderr, "[sfizz] error opening frame\n");
        return false;
    }

    editor->open(*frame);

    for (FObject* update : continuousUpdates_)
        update->addDependent(this);
    for (FObject* update : triggerUpdates_)
        update->addDependent(this);

    Steinberg::IdleUpdateHandler::start();

    for (FObject* update : continuousUpdates_)
        update->deferUpdate();

    absl::optional<fs::path> userFilesDir = SfizzPaths::getSfzConfigDefaultPath();
    uiReceiveValue(EditId::CanEditUserFilesDir, 1);
    uiReceiveValue(EditId::UserFilesDir, userFilesDir.value_or(fs::path()).u8string());
    uiReceiveValue(EditId::FallbackFilesDir, SfizzPaths::getSfzFallbackDefaultPath().u8string());

    return true;
}

void PLUGIN_API SfizzVstEditor::close()
{
    CFrame *frame = this->frame;
    if (frame) {
        Steinberg::IdleUpdateHandler::stop();

        for (FObject* update : continuousUpdates_)
            update->removeDependent(this);
        for (FObject* update : triggerUpdates_)
            update->removeDependent(this);

        if (editor_)
            editor_->close();
        if (frame->getNbReference() != 1)
            frame->forget();
        else
            frame->close();
        this->frame = nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(oscQueueMutex_);
        oscQueue_.reset();
    }
    {
        std::lock_guard<std::mutex> lock(noteEventQueueMutex_);
        noteEventQueue_.reset();
    }
}

///
CMessageResult SfizzVstEditor::notify(CBaseObject* sender, const char* message)
{
    CMessageResult result = VSTGUIEditor::notify(sender, message);

    if (result != kMessageNotified)
        return result;

#if !defined(__APPLE__) && !defined(_WIN32)
    if (message == CVSTGUITimer::kMsgTimer) {
        SharedPointer<VSTGUI::RunLoop> runLoop = RunLoop::get();
        if (runLoop) {
            // note(jpc) I don't find a reliable way to check if the host
            //   notifier of X11 events is working. If there is, remove this and
            //   avoid polluting Linux hosts which implement the loop correctly.
            runLoop->processSomeEvents();

            runLoop->cleanupDeadHandlers();
        }
    }
#endif

    if (message == CVSTGUITimer::kMsgTimer) {
        processOscQueue();
        processNoteEventQueue();
    }

    return result;
}

void PLUGIN_API SfizzVstEditor::update(FUnknown* changedUnknown, int32 message)
{
    if (OSCUpdate* update = FCast<OSCUpdate>(changedUnknown)) {
        // this update is synchronous: may happen from non-UI thread
        uint32 size = update->size();
        if (size > 0) {
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(update->data());
            std::lock_guard<std::mutex> lock(oscQueueMutex_);
            if (OscByteVec* queue = oscQueue_.get())
                std::copy(bytes, bytes + size, std::back_inserter(*queue));
        }
        return;
    }

    if (NoteUpdate* update = FCast<NoteUpdate>(changedUnknown)) {
        // this update is synchronous: may happen from non-UI thread
        uint32 count = update->count();
        if (count > 0) {
            const auto* events = update->events();
            std::lock_guard<std::mutex> lock(noteEventQueueMutex_);
            if (NoteEventsVec* queue = noteEventQueue_.get())
                std::copy(events, events + count, std::back_inserter(*queue));
        }
    }

    if (SfzUpdate* update = FCast<SfzUpdate>(changedUnknown)) {
        const std::string path = update->getPath();
        uiReceiveValue(EditId::SfzFile, path);
    }

    if (SfzDescriptionUpdate* update = FCast<SfzDescriptionUpdate>(changedUnknown)) {
        const InstrumentDescription desc = parseDescriptionBlob(update->getDescription());
        // TODO(jpc) instrument description
    }

    if (ScalaUpdate* update = FCast<ScalaUpdate>(changedUnknown)) {
        const std::string path = update->getPath();
        uiReceiveValue(EditId::ScalaFile, path);
    }

    if (ProcessorStateUpdate* update = FCast<ProcessorStateUpdate>(changedUnknown)) {
        const SfizzVstState state = update->getState();
        uiReceiveValue(EditId::SfzFile, state.sfzFile);
        uiReceiveValue(EditId::Volume, state.volume);
        uiReceiveValue(EditId::Polyphony, state.numVoices);
        uiReceiveValue(EditId::Oversampling, float(1u << state.oversamplingLog2));
        uiReceiveValue(EditId::PreloadSize, state.preloadSize);
        uiReceiveValue(EditId::ScalaFile, state.scalaFile);
        uiReceiveValue(EditId::ScalaRootKey, state.scalaRootKey);
        uiReceiveValue(EditId::TuningFrequency, state.tuningFrequency);
        uiReceiveValue(EditId::StretchTuning, state.stretchedTuning);
    }

    if (PlayStateUpdate* update = FCast<PlayStateUpdate>(changedUnknown)) {
        const SfizzPlayState playState = update->getState();
        uiReceiveValue(EditId::UINumCurves, playState.curves);
        uiReceiveValue(EditId::UINumMasters, playState.masters);
        uiReceiveValue(EditId::UINumGroups, playState.groups);
        uiReceiveValue(EditId::UINumRegions, playState.regions);
        uiReceiveValue(EditId::UINumPreloadedSamples, playState.preloadedSamples);
        uiReceiveValue(EditId::UINumActiveVoices, playState.activeVoices);
        return;
    }

    if (Vst::RangeParameter* param = Steinberg::FCast<Vst::RangeParameter>(changedUnknown)) {
        const Vst::ParamValue value = param->getNormalized();
        const Vst::ParamID id = param->getInfo().id;
        const SfizzRange range = SfizzRange::getForParameter(id);
        switch (id) {
        case kPidVolume:
            uiReceiveValue(EditId::Volume, range.denormalize(value));
            break;
        case kPidNumVoices:
            uiReceiveValue(EditId::Polyphony, range.denormalize(value));
            break;
        case kPidOversampling:
            uiReceiveValue(EditId::Oversampling, float(1u << (int32)range.denormalize(value)));
            break;
        case kPidPreloadSize:
            uiReceiveValue(EditId::PreloadSize, range.denormalize(value));
            break;
        case kPidScalaRootKey:
            uiReceiveValue(EditId::ScalaRootKey, range.denormalize(value));
            break;
        case kPidTuningFrequency:
            uiReceiveValue(EditId::TuningFrequency, range.denormalize(value));
            break;
        case kPidStretchedTuning:
            uiReceiveValue(EditId::StretchTuning, range.denormalize(value));
            break;
        }
        return;
    }

    Vst::VSTGUIEditor::update(changedUnknown, message);
}

void SfizzVstEditor::processOscQueue()
{
    std::lock_guard<std::mutex> lock(oscQueueMutex_);

    OscByteVec* queue = oscQueue_.get();
    if (!queue)
        return;

    const uint8_t* oscData = queue->data();
    size_t oscSize = queue->size();

    const char* path;
    const char* sig;
    const sfizz_arg_t* args;
    uint8_t buffer[1024];

    uint32_t msgSize;
    while ((msgSize = sfizz_extract_message(oscData, oscSize, buffer, sizeof(buffer), &path, &sig, &args)) > 0) {
        uiReceiveMessage(path, sig, args);
        oscData += msgSize;
        oscSize -= msgSize;
    }

    queue->clear();
}

void SfizzVstEditor::processNoteEventQueue()
{
    std::lock_guard<std::mutex> lock(noteEventQueueMutex_);

    NoteEventsVec* queue = noteEventQueue_.get();
    if (!queue)
        return;

    for (std::pair<uint32, float> event : *queue)
        uiReceiveValue(editIdForKey(event.first), event.second);

    queue->clear();
}

///
void SfizzVstEditor::uiSendValue(EditId id, const EditValue& v)
{
    if (id == EditId::SfzFile)
        loadSfzFile(v.to_string());
    else if (id == EditId::ScalaFile)
        loadScalaFile(v.to_string());
    else {
        SfizzVstController* ctrl = getController();

        auto normalizeAndSet = [ctrl](Vst::ParamID pid, float value) {
            float normValue = SfizzRange::getForParameter(pid).normalize(value);
            ctrl->setParamNormalized(pid, normValue);
            ctrl->performEdit(pid, normValue);
        };

        switch (id) {
        case EditId::Volume:
            normalizeAndSet(kPidVolume, v.to_float());
            break;
        case EditId::Polyphony:
            normalizeAndSet(kPidNumVoices, v.to_float());
            break;
        case EditId::Oversampling:
            {
                const int32 factor = static_cast<int32>(v.to_float());
                normalizeAndSet(kPidOversampling, integerLog2(factor));
            }
            break;
        case EditId::PreloadSize:
            normalizeAndSet(kPidPreloadSize, v.to_float());
            break;
        case EditId::ScalaRootKey:
            normalizeAndSet(kPidScalaRootKey, v.to_float());
            break;
        case EditId::TuningFrequency:
            normalizeAndSet(kPidTuningFrequency, v.to_float());
            break;
        case EditId::StretchTuning:
            normalizeAndSet(kPidStretchedTuning, v.to_float());
            break;

        case EditId::UserFilesDir:
            SfizzPaths::setSfzConfigDefaultPath(fs::u8path(v.to_string()));
            break;

        default:
            break;
        }
    }
}

void SfizzVstEditor::uiBeginSend(EditId id)
{
    Vst::ParamID pid = parameterOfEditId(id);
    if (pid != Vst::kNoParamId)
        getController()->beginEdit(pid);
}

void SfizzVstEditor::uiEndSend(EditId id)
{
    Vst::ParamID pid = parameterOfEditId(id);
    if (pid != Vst::kNoParamId)
        getController()->endEdit(pid);
}

void SfizzVstEditor::uiSendMIDI(const uint8_t* data, uint32_t len)
{
    SfizzVstController* ctl = getController();

    Steinberg::OPtr<Vst::IMessage> msg { ctl->allocateMessage() };
    if (!msg) {
        fprintf(stderr, "[Sfizz] UI could not allocate message\n");
        return;
    }

    msg->setMessageID("MidiMessage");
    Vst::IAttributeList* attr = msg->getAttributes();
    attr->setBinary("Data", data, len);
    ctl->sendMessage(msg);
}

void SfizzVstEditor::uiSendMessage(const char* path, const char* sig, const sfizz_arg_t* args)
{
    SfizzVstController* ctl = getController();

    Steinberg::OPtr<Vst::IMessage> msg { ctl->allocateMessage() };
    if (!msg) {
        fprintf(stderr, "[Sfizz] UI could not allocate message\n");
        return;
    }

    uint8_t* oscTemp = oscTemp_.get();
    uint32_t oscSize = sfizz_prepare_message(oscTemp, kOscTempSize, path, sig, args);
    if (oscSize <= kOscTempSize) {
        msg->setMessageID("OscMessage");
        Vst::IAttributeList* attr = msg->getAttributes();
        attr->setBinary("Data", oscTemp, oscSize);
        ctl->sendMessage(msg);
    }
}

///
void SfizzVstEditor::loadSfzFile(const std::string& filePath)
{
    SfizzVstController* ctl = getController();

    Steinberg::OPtr<Vst::IMessage> msg { ctl->allocateMessage() };
    if (!msg) {
        fprintf(stderr, "[Sfizz] UI could not allocate message\n");
        return;
    }

    msg->setMessageID("LoadSfz");
    Vst::IAttributeList* attr = msg->getAttributes();
    attr->setBinary("File", filePath.data(), filePath.size());
    ctl->sendMessage(msg);
}

void SfizzVstEditor::loadScalaFile(const std::string& filePath)
{
    SfizzVstController* ctl = getController();

    Steinberg::OPtr<Vst::IMessage> msg { ctl->allocateMessage() };
    if (!msg) {
        fprintf(stderr, "[Sfizz] UI could not allocate message\n");
        return;
    }

    msg->setMessageID("LoadScala");
    Vst::IAttributeList* attr = msg->getAttributes();
    attr->setBinary("File", filePath.data(), filePath.size());
    ctl->sendMessage(msg);
}

Vst::ParamID SfizzVstEditor::parameterOfEditId(EditId id)
{
    switch (id) {
    case EditId::Volume: return kPidVolume;
    case EditId::Polyphony: return kPidNumVoices;
    case EditId::Oversampling: return kPidOversampling;
    case EditId::PreloadSize: return kPidPreloadSize;
    case EditId::ScalaRootKey: return kPidScalaRootKey;
    case EditId::TuningFrequency: return kPidTuningFrequency;
    case EditId::StretchTuning: return kPidStretchedTuning;
    default: return Vst::kNoParamId;
    }
}
