// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstController.h"
#include "SfizzVstEditor.h"
#include "SfizzVstParameters.h"
#include "SfizzVstIDs.h"
#include "plugin/InstrumentDescription.h"
#include "base/source/fstreamer.h"
#include "base/source/updatehandler.h"
#include <absl/strings/match.h>
#include <ghc/fs_std.hpp>
#include <atomic>
#include <cassert>

enum { kProgramListID = 0 };

tresult PLUGIN_API SfizzVstControllerNoUi::initialize(FUnknown* context)
{
    tresult result = EditControllerEx1::initialize(context);
    if (result != kResultTrue)
        return result;

    // initialize the update handler
    Steinberg::UpdateHandler::instance();

    // initialize the thread checker
    threadChecker_ = Vst::ThreadChecker::create();

    // create update objects
    queuedUpdates_ = Steinberg::owned(new QueuedUpdates);
    sfzUpdate_ = Steinberg::owned(new SfzUpdate);
    sfzDescriptionUpdate_ = Steinberg::owned(new SfzDescriptionUpdate);
    scalaUpdate_ = Steinberg::owned(new ScalaUpdate);
    playStateUpdate_ = Steinberg::owned(new PlayStateUpdate);

    // Unit
    addUnit(new Vst::Unit(Steinberg::String("Root"), Vst::kRootUnitId, Vst::kNoParentUnitId, kProgramListID));

    // Parameters
    Vst::ParamID pid = 0;

    // Ordinary parameters
    parameters.addParameter(
        SfizzRange::getForParameter(kPidVolume).createParameter(
            Steinberg::String("Volume"), pid++, Steinberg::String("dB"),
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidNumVoices).createParameter(
            Steinberg::String("Polyphony"), pid++, nullptr,
            0, Vst::ParameterInfo::kNoFlags, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidOversampling).createParameter(
            Steinberg::String("Oversampling"), pid++, nullptr,
            0, Vst::ParameterInfo::kNoFlags, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidPreloadSize).createParameter(
            Steinberg::String("Preload size"), pid++, nullptr,
            0, Vst::ParameterInfo::kNoFlags, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidScalaRootKey).createParameter(
            Steinberg::String("Scala root key"), pid++, nullptr,
            0, Vst::ParameterInfo::kNoFlags, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidTuningFrequency).createParameter(
            Steinberg::String("Tuning frequency"), pid++, Steinberg::String("Hz"),
            0, Vst::ParameterInfo::kNoFlags, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidStretchedTuning).createParameter(
            Steinberg::String("Stretched tuning"), pid++, nullptr,
            0, Vst::ParameterInfo::kNoFlags, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidSampleQuality).createParameter(
            Steinberg::String("Sample quality"), pid++, nullptr,
            0, Vst::ParameterInfo::kNoFlags, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidOscillatorQuality).createParameter(
            Steinberg::String("Oscillator quality"), pid++, nullptr,
            0, Vst::ParameterInfo::kNoFlags, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidFreewheelingSampleQuality).createParameter(
            Steinberg::String("Freewheeling sample quality"), pid++, nullptr,
            0, Vst::ParameterInfo::kNoFlags, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidFreewheelingOscillatorQuality).createParameter(
            Steinberg::String("Freewheeling oscillator quality"), pid++, nullptr,
            0, Vst::ParameterInfo::kNoFlags, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidSustainCancelsRelease).createParameter(
            Steinberg::String("Sustain cancels release"), pid++, nullptr,
            1, Vst::ParameterInfo::kNoFlags, Vst::kRootUnitId));

    // MIDI special controllers
    parameters.addParameter(
        SfizzRange::getForParameter(kPidAftertouch).createParameter(
            Steinberg::String("Aftertouch"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidPitchBend).createParameter(
            Steinberg::String("Pitch bend"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));

    // MIDI controllers
    for (unsigned i = 0; i < sfz::config::numCCs; ++i) {
        Steinberg::String title;
        Steinberg::String shortTitle;
        title.printf("Controller %u", i);
        shortTitle.printf("CC%u", i);

        parameters.addParameter(
            SfizzRange::getForParameter(kPidCC0 + i).createParameter(
                title, pid++, nullptr, 0, Vst::ParameterInfo::kCanAutomate,
                Vst::kRootUnitId, shortTitle));
    }

    // Volume levels
    for (unsigned i = 0; i < 16; ++i) {
        Steinberg::String title;
        title.printf("Level %u", i);

        parameters.addParameter(
            SfizzRange::getForParameter(kPidLevel0 + i).createParameter(
                title, pid++, nullptr, 0,
                Vst::ParameterInfo::kIsReadOnly|Vst::ParameterInfo::kIsHidden, Vst::kRootUnitId));
    }

    // Editor status
    parameters.addParameter(
        SfizzRange::getForParameter(kPidEditorOpen).createParameter(
            Steinberg::String("Editor open"), pid++, nullptr,
            0, Vst::ParameterInfo::kIsReadOnly|Vst::ParameterInfo::kIsHidden, Vst::kRootUnitId));

    // Initial MIDI mapping
    for (int32 i = 0; i < Vst::kCountCtrlNumber; ++i) {
        Vst::ParamID id = Vst::kNoParamId;
        switch (i) {
        case Vst::kAfterTouch:
            id = kPidAftertouch;
            break;
        case Vst::kPitchBend:
            id = kPidPitchBend;
            break;
        default:
            if (i < 128)
                id = kPidCC0 + i;
            break;
        }
        midiMapping_[i] = id;
    }

    // Program list
    IPtr<Vst::ProgramListWithPitchNames> list = Steinberg::owned(
        new Vst::ProgramListWithPitchNames(Steinberg::String("Programs"), kProgramListID, Vst::kRootUnitId));
    list->addProgram(Steinberg::String("Default"));
    addProgramList(list);
    list->addRef();

    // Use linear knobs
    setKnobMode(kLinearMode);

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstControllerNoUi::terminate()
{
    return EditControllerEx1::terminate();
}

tresult PLUGIN_API SfizzVstControllerNoUi::getMidiControllerAssignment(int32 busIndex, int16 channel, Vst::CtrlNumber midiControllerNumber, Vst::ParamID& id)
{
    if (midiControllerNumber < 0 || midiControllerNumber >= Vst::kCountCtrlNumber) {
        id = Vst::kNoParamId;
        return kResultFalse;
    }

    id = midiMapping_[midiControllerNumber];
    if (id == Vst::kNoParamId)
        return kResultFalse;

    return kResultTrue;
}

int32 PLUGIN_API SfizzVstControllerNoUi::getKeyswitchCount(int32 busIndex, int16 channel)
{
    (void)channel;

    if (busIndex != 0)
        return 0;

    return keyswitches_.size();
}

tresult PLUGIN_API SfizzVstControllerNoUi::getKeyswitchInfo(int32 busIndex, int16 channel, int32 keySwitchIndex, Vst::KeyswitchInfo& info)
{
    (void)channel;

    if (busIndex != 0)
        return kResultFalse;

    if (keySwitchIndex < 0 || keySwitchIndex >= keyswitches_.size())
        return kResultFalse;

    info = keyswitches_[keySwitchIndex];
    return kResultTrue;
}

tresult PLUGIN_API SfizzVstControllerNoUi::beginEditFromHost(Vst::ParamID paramID)
{
    // Note(jpc) implementing this interface is a workaround to make
    //           non-automatable parameters editable in Ardour (as of 6.6)
    (void)paramID;
    return kResultTrue;
}

tresult PLUGIN_API SfizzVstControllerNoUi::endEditFromHost(Vst::ParamID paramID)
{
    (void)paramID;
    return kResultTrue;
}

tresult PLUGIN_API SfizzVstControllerNoUi::getParamStringByValue(Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
    switch (tag) {
    case kPidOversampling:
        {
            const SfizzRange range = SfizzRange::getForParameter(tag);
            const int factorLog2 = static_cast<int>(range.denormalize(valueNormalized));
            Steinberg::String buf;
            buf.printf("%dX", 1 << factorLog2);
            buf.copyTo(string);
            return kResultTrue;
        }
    }

    return EditControllerEx1::getParamStringByValue(tag, valueNormalized, string);
}

tresult PLUGIN_API SfizzVstControllerNoUi::getParamValueByString(Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
    switch (tag) {
    case kPidOversampling:
        {
            int32 factor;
            if (!Steinberg::String::scanInt32(string, factor, false))
                factor = 1;

            const SfizzRange range = SfizzRange::getForParameter(tag);
            valueNormalized = range.normalize(integerLog2(factor));
            return kResultTrue;
        }
    }

    return EditControllerEx1::getParamValueByString(tag, string, valueNormalized);
}

tresult SfizzVstControllerNoUi::setParam(Vst::ParamID tag, float value)
{
    const SfizzRange range = SfizzRange::getForParameter(tag);
    return setParamNormalized(tag, range.normalize(value));
}

tresult PLUGIN_API SfizzVstControllerNoUi::setComponentState(IBStream* stream)
{
    SfizzVstState s;

    tresult r = s.load(stream);
    if (r != kResultTrue)
        return r;

    setParam(kPidVolume, s.volume);
    setParam(kPidNumVoices, s.numVoices);
    setParam(kPidOversampling, s.oversamplingLog2);
    setParam(kPidPreloadSize, s.preloadSize);
    setParam(kPidScalaRootKey, s.scalaRootKey);
    setParam(kPidTuningFrequency, s.tuningFrequency);
    setParam(kPidStretchedTuning, s.stretchedTuning);
    setParam(kPidSampleQuality, s.sampleQuality);
    setParam(kPidOscillatorQuality, s.oscillatorQuality);
    setParam(kPidFreewheelingSampleQuality, s.freewheelingSampleQuality);
    setParam(kPidFreewheelingOscillatorQuality, s.freewheelingOscillatorQuality);
    setParam(kPidSustainCancelsRelease, s.sustainCancelsRelease);

    uint32 ccLimit = uint32(std::min(s.controllers.size(), size_t(sfz::config::numCCs)));
    for (uint32 cc = 0; cc < ccLimit; ++cc) {
        if (absl::optional<float> value = s.controllers[cc])
            setParam(kPidCC0 + cc, *value);
    }

    sfzUpdate_->setPath(s.sfzFile);
    sfzUpdate_->deferUpdate();
    scalaUpdate_->setPath(s.scalaFile);
    scalaUpdate_->deferUpdate();

    return kResultTrue;
}

tresult SfizzVstControllerNoUi::notify(Vst::IMessage* message)
{
    // Note: is expected to be called from the controller thread only

    tresult result = EditControllerEx1::notify(message);
    if (result != kResultFalse)
        return result;

    if (!threadChecker_->test()) {
        static std::atomic_bool warn_once_flag { false };
        if (!warn_once_flag.exchange(true))
            fprintf(stderr, "[sfizz] controller notification arrives from the wrong thread\n");
    }

    const char* id = message->getMessageID();

    ///
    if (!strcmp(id, SfzUpdate::getFClassID())) {
        if (!sfzUpdate_->convertFromMessage(*message)) {
            // assert(false);
            return kResultFalse;
        }

        // update the program name and notify
        std::string name = fs::u8path(sfzUpdate_->getPath()).filename().u8string();
        if (absl::EndsWithIgnoreCase(name, ".sfz"))
            name.resize(name.size() - 4);
        setProgramName(kProgramListID, 0, Steinberg::String(name.c_str()));

        FUnknownPtr<Vst::IUnitHandler> unitHandler(getComponentHandler());
        if (unitHandler)
            unitHandler->notifyProgramListChange(kProgramListID, 0);

        //
        sfzUpdate_->deferUpdate();
    }
    else if (!strcmp(id, SfzDescriptionUpdate::getFClassID())) {
        if (!sfzDescriptionUpdate_->convertFromMessage(*message)) {
            assert(false);
            return kResultFalse;
        }

        // parse the description blob
        const InstrumentDescription desc = parseDescriptionBlob(
            sfzDescriptionUpdate_->getDescription());

        // update pitch names and notify
        Vst::ProgramListWithPitchNames* list =
            static_cast<Vst::ProgramListWithPitchNames*>(getProgramList(kProgramListID));
        for (int16 pitch = 0; pitch < 128; ++pitch) {
            Steinberg::String pitchName;
            if (desc.keyUsed.test(pitch) && !desc.keyLabel[pitch].empty())
                pitchName = Steinberg::String(desc.keyLabel[pitch].c_str());
            else if (desc.keyswitchUsed.test(pitch) && !desc.keyswitchLabel[pitch].empty())
                pitchName = Steinberg::String(desc.keyswitchLabel[pitch].c_str());

            list->setPitchName(0, pitch, pitchName);
        }

        FUnknownPtr<Vst::IUnitHandler> unitHandler(getComponentHandler());
        if (unitHandler)
            unitHandler->notifyProgramListChange(kProgramListID, 0);

        // update the key switches and notify
        size_t idKeyswitch = 0;
        for (int16 pitch = 0; pitch < 128; ++pitch)
            idKeyswitch += desc.keyswitchUsed.test(pitch);
        keyswitches_.resize(idKeyswitch);

        idKeyswitch = 0;
        for (int16 pitch = 0; pitch < 128; ++pitch) {
            if (!desc.keyswitchUsed.test(pitch))
                continue;
            Vst::KeyswitchInfo info {};
            info.typeId = Vst::kNoteOnKeyswitchTypeID;
            Steinberg::String(desc.keyswitchLabel[pitch].c_str()).copyTo(info.title);
            Steinberg::String(desc.keyswitchLabel[pitch].c_str()).copyTo(info.shortTitle);
            info.keyswitchMin = pitch; // TODO reexamine this when supporting keyswitch groups
            info.keyswitchMax = pitch; // TODO reexamine this when supporting keyswitch groups
            info.keyRemapped = pitch;
            info.unitId = Vst::kRootUnitId;
            info.flags = 0;
            keyswitches_[idKeyswitch++] = info;
        }

        if (Vst::IComponentHandler* componentHandler = getComponentHandler())
            componentHandler->restartComponent(Vst::kKeyswitchChanged);

        // update the parameter titles and notify
        for (uint32 cc = 0; cc < sfz::config::numCCs; ++cc) {
            Vst::ParamID pid = kPidCC0 + cc;
            Vst::Parameter* param = getParameterObject(pid);
            Vst::ParameterInfo& info = param->getInfo();
            Steinberg::String title;
            Steinberg::String shortTitle;
            if (!desc.ccLabel[cc].empty()) {
                title = desc.ccLabel[cc].c_str();
                shortTitle = title;
            }
            else {
                title.printf("Controller %u", cc);
                shortTitle.printf("CC%u", cc);
            }
            title.copyTo(info.title);
            shortTitle.copyTo(info.shortTitle);
        }

        if (Vst::IComponentHandler* componentHandler = getComponentHandler())
            componentHandler->restartComponent(Vst::kParamTitlesChanged);

        //
        sfzDescriptionUpdate_->deferUpdate();
    }
    else if (!strcmp(id, ScalaUpdate::getFClassID())) {
        if (!scalaUpdate_->convertFromMessage(*message)) {
            assert(false);
            return kResultFalse;
        }
        scalaUpdate_->deferUpdate();
    }
    else if (!strcmp(id, PlayStateUpdate::getFClassID())) {
        if (!playStateUpdate_->convertFromMessage(*message)) {
            assert(false);
            return kResultFalse;
        }
        playStateUpdate_->deferUpdate();
    }
    else if (!strcmp(id, OSCUpdate::getFClassID())) {
        IPtr<OSCUpdate> update = OSCUpdate::createFromMessage(*message);
        if (!update) {
            assert(false);
            return kResultFalse;
        }
        queuedUpdates_->enqueue(update);
        queuedUpdates_->deferUpdate();
    }
    else if (!strcmp(id, NoteUpdate::getFClassID())) {
        IPtr<NoteUpdate> update = NoteUpdate::createFromMessage(*message);
        if (!update) {
            assert(false);
            return kResultFalse;
        }
        queuedUpdates_->enqueue(update);
        queuedUpdates_->deferUpdate();
    }
    else if (!strcmp(id, AutomationUpdate::getFClassID())) {
        IPtr<AutomationUpdate> update = AutomationUpdate::createFromMessage(*message);
        if (!update) {
            assert(false);
            return kResultFalse;
        }
        for (AutomationUpdate::Item item : update->getItems())
            setParam(item.first, item.second);
    }

    return result;
}

// --- Controller with UI --- //

IPlugView* PLUGIN_API SfizzVstController::createView(FIDString _name)
{
    ConstString name(_name);

    fprintf(stderr, "[sfizz] about to create view: %s\n", _name);

    if (name != Vst::ViewType::kEditor)
        return nullptr;

    std::vector<FObject*> updates;
    updates.push_back(queuedUpdates_);
    updates.push_back(sfzUpdate_);
    updates.push_back(sfzDescriptionUpdate_);
    updates.push_back(scalaUpdate_);
    updates.push_back(playStateUpdate_);
    for (uint32 i = 0, n = parameters.getParameterCount(); i < n; ++i)
        updates.push_back(parameters.getParameterByIndex(i));

    IPtr<SfizzVstEditor> editor = Steinberg::owned(
        new SfizzVstEditor(this, absl::MakeSpan(updates)));

    editor->remember();
    return editor;
}

FUnknown* SfizzVstController::createInstance(void*)
{
    return static_cast<Vst::IEditController*>(new SfizzVstController);
}

template <>
FUnknown* createInstance<SfizzVstController>(void* context)
{
    return SfizzVstController::createInstance(context);
}

FUID SfizzVstController::cid = SfizzVstController_cid;
