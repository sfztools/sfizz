// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstEditor.h"
#include "SfizzVstState.h"
#include "SfizzVstParameters.h"
#include "SfizzVstUpdates.h"
#include "editor/Editor.h"
#include "editor/EditIds.h"
#include "plugin/SfizzFileScan.h"
#include "plugin/InstrumentDescription.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "IdleUpdateHandler.h"
#if !defined(__APPLE__) && !defined(_WIN32)
#include "X11RunLoop.h"
#endif
#include <ghc/fs_std.hpp>
#include <atomic>

using namespace VSTGUI;

static ViewRect sfizzUiViewRect { 0, 0, Editor::viewWidth, Editor::viewHeight };

enum {
    kOscTempSize = 8192,
    kOscQueueSize = 65536,
    kNoteEventQueueSize = 8192,
};

SfizzVstEditor::SfizzVstEditor(SfizzVstController* controller, absl::Span<FObject*> updates)
    : VSTGUIEditor(controller, &sfizzUiViewRect),
      oscTemp_(new uint8_t[kOscTempSize]),
      updates_(updates.begin(), updates.end())
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

    Editor* editor = new Editor(*this);
    editor_.reset(editor);

    if (!frame->open(parent, platformType, config)) {
        fprintf(stderr, "[sfizz] error opening frame\n");
        return false;
    }

    editor->open(*frame);

    for (FObject* update : updates_)
        update->addDependent(this);

    threadChecker_ = Vst::ThreadChecker::create();

    parametersToUpdate_.clear();

    Steinberg::IdleUpdateHandler::start();

    for (FObject* update : updates_)
        update->deferUpdate();

    // let the editor know about plugin format
    absl::string_view pluginFormatName = "VST3";

    if (FUnknownPtr<Vst::IHostApplication> app { controller->getHostContext() }) {
        Vst::String128 name;
        app->getName(name);
        uiReceiveValue(EditId::PluginHost, std::string(Steinberg::String(name).text8()));

        void* interfacePtr;
        if (app->queryInterface(Vst::IVst3ToAUWrapper_iid, &interfacePtr) == kResultTrue)
            pluginFormatName = "Audio Unit";
        else if (app->queryInterface(Vst::IVst3ToVst2Wrapper_iid, &interfacePtr) == kResultTrue)
            pluginFormatName = "VST2";
        else if (app->queryInterface(Vst::IVst3ToAAXWrapper_iid, &interfacePtr) == kResultTrue)
            pluginFormatName = "AAX";
    }

    uiReceiveValue(EditId::PluginFormat, std::string(pluginFormatName));

    absl::optional<fs::path> userFilesDir = SfizzPaths::getSfzConfigDefaultPath();
    uiReceiveValue(EditId::CanEditUserFilesDir, 1);
    uiReceiveValue(EditId::UserFilesDir, userFilesDir.value_or(fs::path()).u8string());
    uiReceiveValue(EditId::FallbackFilesDir, SfizzPaths::getSfzFallbackDefaultPath().u8string());

    updateEditorIsOpenParameter();

    return true;
}

void PLUGIN_API SfizzVstEditor::close()
{
    CFrame *frame = this->frame;
    if (frame) {
        Steinberg::IdleUpdateHandler::stop();

        for (FObject* update : updates_)
            update->removeDependent(this);

        if (editor_) {
            editor_->close();
            editor_ = nullptr;
        }

        if (frame->getNbReference() != 1)
            frame->forget();
        else {
            frame->close();
#if !defined(__APPLE__) && !defined(_WIN32)
            // if vstgui is done using the runloop, destroy it
            if (!RunLoop::get())
                _runLoop = nullptr;
#endif
        }
        this->frame = nullptr;
    }

    updateEditorIsOpenParameter();
}

void SfizzVstEditor::updateEditorIsOpenParameter()
{
    SfizzVstController* ctrl = getController();
    bool editorIsOpen = frame && frame->isVisible();
    ctrl->setParamNormalized(kPidEditorOpen, editorIsOpen);
    ctrl->performEdit(kPidEditorOpen, editorIsOpen);
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
        }
    }
#endif

    if (message == CVSTGUITimer::kMsgTimer) {
        processParameterUpdates();
        updateEditorIsOpenParameter(); // Note(jpc) for Reaper, it can fail at open time
    }

    return result;
}

void PLUGIN_API SfizzVstEditor::update(FUnknown* changedUnknown, int32 message)
{
    if (processUpdate(changedUnknown, message))
        return;

    Vst::VSTGUIEditor::update(changedUnknown, message);
}

bool SfizzVstEditor::processUpdate(FUnknown* changedUnknown, int32 message)
{
    if (QueuedUpdates* update = FCast<QueuedUpdates>(changedUnknown)) {
        for (FObject* queuedUpdate : update->getUpdates(this))
            processUpdate(queuedUpdate, message);
        return true;
    }

    if (OSCUpdate* update = FCast<OSCUpdate>(changedUnknown)) {
        const uint8* oscData = update->data();
        uint32 oscSize = update->size();

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

        return true;
    }

    if (NoteUpdate* update = FCast<NoteUpdate>(changedUnknown)) {
        const NoteUpdate::Item* events = update->events();
        uint32 count = update->count();
        for (uint32 i = 0; i < count; ++i)
            uiReceiveValue(editIdForKey(events[i].first), events[i].second);
        return true;
    }

    if (SfzUpdate* update = FCast<SfzUpdate>(changedUnknown)) {
        const std::string path = update->getPath();
        uiReceiveValue(EditId::SfzFile, path);
        return true;
    }

    if (SfzDescriptionUpdate* update = FCast<SfzDescriptionUpdate>(changedUnknown)) {
        const InstrumentDescription desc = parseDescriptionBlob(update->getDescription());

        uiReceiveValue(EditId::UINumCurves, desc.numCurves);
        uiReceiveValue(EditId::UINumMasters, desc.numMasters);
        uiReceiveValue(EditId::UINumGroups, desc.numGroups);
        uiReceiveValue(EditId::UINumRegions, desc.numRegions);
        uiReceiveValue(EditId::UINumPreloadedSamples, desc.numSamples);

        const fs::path rootPath = fs::u8path(desc.rootPath);
        const fs::path imagePath = rootPath / fs::u8path(desc.image);
        uiReceiveValue(EditId::BackgroundImage, imagePath.u8string());

        const fs::path ctrlImagePath = rootPath / fs::u8path(desc.image_controls);
        uiReceiveValue(EditId::ControlsImage, ctrlImagePath.u8string());

        for (unsigned key = 0; key < 128; ++key) {
            bool keyUsed = desc.keyUsed.test(key);
            bool keyswitchUsed = desc.keyswitchUsed.test(key);
            uiReceiveValue(editIdForKeyUsed(int(key)), float(keyUsed));
            uiReceiveValue(editIdForKeyswitchUsed(int(key)), float(keyswitchUsed));
            if (keyUsed)
                uiReceiveValue(editIdForKeyLabel(int(key)), desc.keyLabel[key]);
            if (keyswitchUsed)
                uiReceiveValue(editIdForKeyswitchLabel(int(key)), desc.keyswitchLabel[key]);
        }

        for (unsigned cc = 0; cc < sfz::config::numCCs; ++cc) {
            bool ccUsed = desc.ccUsed.test(cc) && !desc.sustainOrSostenuto.test(cc);
            uiReceiveValue(editIdForCCUsed(int(cc)), float(ccUsed));
            if (ccUsed) {
                uiReceiveValue(editIdForCCDefault(int(cc)), desc.ccDefault[cc]);
                uiReceiveValue(editIdForCCLabel(int(cc)), desc.ccLabel[cc]);
            }
        }
        return true;
    }

    if (ScalaUpdate* update = FCast<ScalaUpdate>(changedUnknown)) {
        const std::string path = update->getPath();
        uiReceiveValue(EditId::ScalaFile, path);
        return true;
    }

    if (PlayStateUpdate* update = FCast<PlayStateUpdate>(changedUnknown)) {
        const SfizzPlayState playState = update->getState();
        uiReceiveValue(EditId::UINumActiveVoices, playState.activeVoices);
        return true;
    }

    if (Vst::RangeParameter* param = Steinberg::FCast<Vst::RangeParameter>(changedUnknown)) {
        // Note(jpc) some hosts send us the parameters in the wrong thread...
        //           store these parameters thread-safely and let the idle
        //           callback process them later
        if (threadChecker_->test())
            updateParameter(param);
        else {
            static std::atomic_bool warn_once_flag { false };
            if (!warn_once_flag.exchange(true))
                fprintf(stderr, "[sfizz] using a thread-safety workaround for parameter updates\n");
            const Vst::ParamID id = param->getInfo().id;
            std::lock_guard<std::mutex> lock(parametersToUpdateMutex_);
            parametersToUpdate_.insert(id);
        }
        return true;
    }

    return false;
}

void SfizzVstEditor::processParameterUpdates()
{
    auto extractNextParamID = [this]() -> Vst::ParamID {
        Vst::ParamID id = Vst::kNoParamId;
        std::lock_guard<std::mutex> lock(parametersToUpdateMutex_);
        auto it = parametersToUpdate_.begin();
        if (it != parametersToUpdate_.end()) {
            id = *it;
            parametersToUpdate_.erase(it);
        }
        return id;
    };

    for (Vst::ParamID id; (id = extractNextParamID()) != Vst::kNoParamId; )
        updateParameter(getController()->getParameterObject(id));
}

void SfizzVstEditor::updateParameter(Vst::Parameter* parameterToUpdate)
{
    if (Vst::RangeParameter* param = FCast<Vst::RangeParameter>(parameterToUpdate)) {
        const Vst::ParamID id = param->getInfo().id;
        const Vst::ParamValue value = param->getNormalized();
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
        case kPidSampleQuality:
            uiReceiveValue(EditId::SampleQuality, range.denormalize(value));
            break;
        case kPidOscillatorQuality:
            uiReceiveValue(EditId::OscillatorQuality, range.denormalize(value));
            break;
        case kPidFreewheelingSampleQuality:
            uiReceiveValue(EditId::FreewheelingSampleQuality, range.denormalize(value));
            break;
        case kPidFreewheelingOscillatorQuality:
            uiReceiveValue(EditId::FreewheelingOscillatorQuality, range.denormalize(value));
            break;
        case kPidSustainCancelsRelease:
            uiReceiveValue(EditId::SustainCancelsRelease, range.denormalize(value));
            break;
        case kPidNumOutputs:
            uiReceiveValue(EditId::PluginOutputs, (int32)range.denormalize(value));
            break;
        default:
            if (id >= kPidCC0 && id <= kPidCCLast) {
                int cc = int(id - kPidCC0);
                uiReceiveValue(editIdForCC(cc), range.denormalize(value));
            } else if (id >= kPidLevel0 && id <= kPidLevelLast) {
                int levelId = int(id - kPidLevel0);
                uiReceiveValue(editIdForLevel(levelId), range.denormalize(value));
            }
            break;
        }
    }
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
        case EditId::SampleQuality:
            normalizeAndSet(kPidSampleQuality, v.to_float());
            break;
        case EditId::OscillatorQuality:
            normalizeAndSet(kPidOscillatorQuality, v.to_float());
            break;
        case EditId::FreewheelingSampleQuality:
            normalizeAndSet(kPidFreewheelingSampleQuality, v.to_float());
            break;
        case EditId::FreewheelingOscillatorQuality:
            normalizeAndSet(kPidFreewheelingOscillatorQuality, v.to_float());
            break;
        case EditId::SustainCancelsRelease:
            normalizeAndSet(kPidSustainCancelsRelease, v.to_float());
            break;
        case EditId::UserFilesDir:
            SfizzPaths::setSfzConfigDefaultPath(fs::u8path(v.to_string()));
            break;

        default:
            if (editIdIsCC(id))
                normalizeAndSet(kPidCC0 + ccForEditId(id), v.to_float());
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
    case EditId::SampleQuality: return kPidSampleQuality;
    case EditId::OscillatorQuality: return kPidOscillatorQuality;
    default:
        if (editIdIsCC(id))
            return kPidCC0 + ccForEditId(id);
        else if (editIdIsLevel(id))
            return kPidLevel0 + levelForEditId(id);
        return Vst::kNoParamId;
    }
}
