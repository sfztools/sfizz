// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstController.h"
#include "SfizzVstEditor.h"
#include "SfizzVstParameters.h"
#include "base/source/fstreamer.h"
#include "base/source/updatehandler.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

tresult PLUGIN_API SfizzVstControllerNoUi::initialize(FUnknown* context)
{
    tresult result = EditController::initialize(context);
    if (result != kResultTrue)
        return result;

    // initialize the update handler
    Steinberg::UpdateHandler::instance();

    // create update objects
    oscUpdate_ = Steinberg::owned(new OSCUpdate);
    sfzPathUpdate_ = Steinberg::owned(new FilePathUpdate(kFilePathUpdateSfz));
    scalaPathUpdate_ = Steinberg::owned(new FilePathUpdate(kFilePathUpdateScala));
    processorStateUpdate_ = Steinberg::owned(new ProcessorStateUpdate);
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
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidOversampling).createParameter(
            Steinberg::String("Oversampling"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidPreloadSize).createParameter(
            Steinberg::String("Preload size"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidScalaRootKey).createParameter(
            Steinberg::String("Scala root key"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidTuningFrequency).createParameter(
            Steinberg::String("Tuning frequency"), pid++, Steinberg::String("Hz"),
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        SfizzRange::getForParameter(kPidStretchedTuning).createParameter(
            Steinberg::String("Stretched tuning"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));

    // MIDI special controllers
    parameters.addParameter(Steinberg::String("Aftertouch"), nullptr, 0, 0.5, 0, pid++, Vst::kRootUnitId);
    parameters.addParameter(Steinberg::String("Pitch bend"), nullptr, 0, 0.5, 0, pid++, Vst::kRootUnitId);

    // MIDI controllers
    for (unsigned i = 0; i < kNumControllerParams; ++i) {
        Steinberg::String title;
        Steinberg::String shortTitle;
        title.printf("Controller %u", i);
        shortTitle.printf("CC%u", i);

        parameters.addParameter(
            title, nullptr, 0, 0, Vst::ParameterInfo::kNoFlags,
            pid++, Vst::kRootUnitId, shortTitle);
    }

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstControllerNoUi::terminate()
{
    return EditController::terminate();
}

tresult PLUGIN_API SfizzVstControllerNoUi::getMidiControllerAssignment(int32 busIndex, int16 channel, Vst::CtrlNumber midiControllerNumber, Vst::ParamID& id)
{
    switch (midiControllerNumber) {
    case Vst::kAfterTouch:
        id = kPidMidiAftertouch;
        return kResultTrue;

    case Vst::kPitchBend:
        id = kPidMidiPitchBend;
        return kResultTrue;

    default:
        if (midiControllerNumber < 0 || midiControllerNumber >= kNumControllerParams)
            return kResultFalse;

        id = kPidMidiCC0 + midiControllerNumber;
        return kResultTrue;
    }
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

    return EditController::getParamStringByValue(tag, valueNormalized, string);
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

    return EditController::getParamValueByString(tag, string, valueNormalized);
}

tresult SfizzVstControllerNoUi::setParam(Vst::ParamID tag, float value)
{
    const SfizzRange range = SfizzRange::getForParameter(tag);
    return setParamNormalized(tag, range.normalize(value));
}

tresult PLUGIN_API SfizzVstControllerNoUi::setParamNormalized(Vst::ParamID tag, Vst::ParamValue normValue)
{
    tresult r = EditController::setParamNormalized(tag, normValue);
    if (r != kResultTrue)
        return r;

    processorStateUpdate_->access([tag, normValue](SfizzVstState& state) {
        const SfizzRange range = SfizzRange::getForParameter(tag);
        switch (tag) {
        case kPidVolume:
            state.volume = range.denormalize(normValue);
            break;
        case kPidNumVoices:
            state.numVoices = (int32)range.denormalize(normValue);
            break;
        case kPidOversampling:
            state.oversamplingLog2 = (int32)range.denormalize(normValue);
            break;
        case kPidPreloadSize:
            state.preloadSize = (int32)range.denormalize(normValue);
            break;
        case kPidScalaRootKey:
            state.scalaRootKey = (int32)range.denormalize(normValue);
            break;
        case kPidTuningFrequency:
            state.tuningFrequency = (float)range.denormalize(normValue);
            break;
        case kPidStretchedTuning:
            state.stretchedTuning = range.denormalize(normValue);
            break;
        }
    });

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstControllerNoUi::setComponentState(IBStream* stream)
{
    SfizzVstState s;

    tresult r = s.load(stream);
    if (r != kResultTrue)
        return r;

    processorStateUpdate_->setState(s);

    setParam(kPidVolume, s.volume);
    setParam(kPidNumVoices, s.numVoices);
    setParam(kPidOversampling, s.oversamplingLog2);
    setParam(kPidPreloadSize, s.preloadSize);
    setParam(kPidScalaRootKey, s.scalaRootKey);
    setParam(kPidTuningFrequency, s.tuningFrequency);
    setParam(kPidStretchedTuning, s.stretchedTuning);

    sfzPathUpdate_->setPath(s.sfzFile);
    sfzPathUpdate_->deferUpdate();
    scalaPathUpdate_->setPath(s.scalaFile);
    scalaPathUpdate_->deferUpdate();

    return kResultTrue;
}

tresult SfizzVstControllerNoUi::notify(Vst::IMessage* message)
{
    // Note: may be called from any thread (Reaper)

    tresult result = EditController::notify(message);
    if (result != kResultFalse)
        return result;

    const char* id = message->getMessageID();
    Vst::IAttributeList* attr = message->getAttributes();

    if (!strcmp(id, "LoadedSfz")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("File", data, size);

        if (result != kResultTrue)
            return result;

        std::string sfzFile(static_cast<const char *>(data), size);
        processorStateUpdate_->access([&sfzFile](SfizzVstState& state) {
            state.sfzFile = sfzFile;
        });
        sfzPathUpdate_->setPath(std::move(sfzFile));
        sfzPathUpdate_->deferUpdate();
    }
    else if (!strcmp(id, "LoadedScala")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("File", data, size);

        if (result != kResultTrue)
            return result;

        std::string scalaFile(static_cast<const char *>(data), size);
        processorStateUpdate_->access([&scalaFile](SfizzVstState& state) {
            state.scalaFile = scalaFile;
        });
        scalaPathUpdate_->setPath(std::move(scalaFile));
        scalaPathUpdate_->deferUpdate();
    }
    else if (!strcmp(id, "NotifiedPlayState")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("PlayState", data, size);

        if (result != kResultTrue)
            return result;

        playStateUpdate_->setState(*static_cast<const SfizzPlayState*>(data));
        playStateUpdate_->deferUpdate();
    }
    else if (!strcmp(id, "ReceivedMessage")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("Message", data, size);

        if (result != kResultTrue)
            return result;

        // this is a synchronous send, because the update object gets reused
        oscUpdate_->setMessage(data, size, false);
        oscUpdate_->changed();
        oscUpdate_->clear();
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

    std::vector<FObject*> continuousUpdates;
    continuousUpdates.push_back(sfzPathUpdate_);
    continuousUpdates.push_back(scalaPathUpdate_);
    continuousUpdates.push_back(playStateUpdate_);
    for (uint32 i = 0, n = parameters.getParameterCount(); i < n; ++i)
        continuousUpdates.push_back(parameters.getParameterByIndex(i));

    std::vector<FObject*> triggerUpdates;
    triggerUpdates.push_back(oscUpdate_);

    IPtr<SfizzVstEditor> editor = Steinberg::owned(
        new SfizzVstEditor(this, absl::MakeSpan(continuousUpdates), absl::MakeSpan(triggerUpdates)));

    editor->remember();
    return editor;
}

FUnknown* SfizzVstController::createInstance(void*)
{
    return static_cast<Vst::IEditController*>(new SfizzVstController);
}

/*
  Note(jpc) Generated at random with uuidgen.
  Can't find docs on it... maybe it's to register somewhere?
 */
FUID SfizzVstController::cid(0x7129736c, 0xbc784134, 0xbb899d56, 0x2ebafe4f);
