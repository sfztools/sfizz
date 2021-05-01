// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstController.h"
#include "SfizzVstEditor.h"
#include "SfizzVstParameters.h"
#include "SfizzVstIDs.h"
#include "base/source/fstreamer.h"
#include "base/source/updatehandler.h"
#include <atomic>
#include <cassert>

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
    parameters.addParameter(
        SfizzRange::getForParameter(kPidLeftLevel).createParameter(
            Steinberg::String("Left level"), pid++, nullptr,
            0, Vst::ParameterInfo::kIsReadOnly|Vst::ParameterInfo::kIsHidden, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidRightLevel).createParameter(
            Steinberg::String("Right level"), pid++, nullptr,
            0, Vst::ParameterInfo::kIsReadOnly|Vst::ParameterInfo::kIsHidden, Vst::kRootUnitId));

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
            assert(false);
            return kResultFalse;
        }
        sfzUpdate_->deferUpdate();
    }
    else if (!strcmp(id, SfzDescriptionUpdate::getFClassID())) {
        if (!sfzDescriptionUpdate_->convertFromMessage(*message)) {
            assert(false);
            return kResultFalse;
        }
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
