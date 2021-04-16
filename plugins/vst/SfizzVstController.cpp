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

tresult PLUGIN_API SfizzVstControllerNoUi::initialize(FUnknown* context)
{
    tresult result = EditController::initialize(context);
    if (result != kResultTrue)
        return result;

    // initialize the update handler
    Steinberg::UpdateHandler::instance();

    // create update objects
    oscUpdate_ = Steinberg::owned(new OSCUpdate);
    noteUpdate_ = Steinberg::owned(new NoteUpdate);
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
    return EditController::terminate();
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
    // Note: may be called from any thread (Reaper)

    tresult result = EditController::notify(message);
    if (result != kResultFalse)
        return result;

    const char* id = message->getMessageID();
    Vst::IAttributeList* attr = message->getAttributes();

    ///
    auto stringFromBinaryAttribute = [attr](const char* id, absl::string_view& string) -> tresult {
        const void* data = nullptr;
        uint32 size = 0;
        tresult result = attr->getBinary(id, data, size);
        if (result == kResultTrue)
            string = absl::string_view(reinterpret_cast<const char*>(data), size);
        return result;
    };

    ///
    if (!strcmp(id, "LoadedSfz")) {
        absl::string_view sfzFile;
        absl::string_view sfzDescriptionBlob;

        result = stringFromBinaryAttribute("File", sfzFile);
        if (result != kResultTrue)
            return result;

        result = stringFromBinaryAttribute("Description", sfzDescriptionBlob);
        if (result != kResultTrue)
            return result;

        sfzUpdate_->setPath(std::string(sfzFile));
        sfzUpdate_->deferUpdate();
        sfzDescriptionUpdate_->setDescription(std::string(sfzDescriptionBlob));
        sfzDescriptionUpdate_->deferUpdate();
    }
    else if (!strcmp(id, "LoadedScala")) {
        absl::string_view scalaFile;

        result = stringFromBinaryAttribute("File", scalaFile);
        if (result != kResultTrue)
            return result;

        scalaUpdate_->setPath(std::string(scalaFile));
        scalaUpdate_->deferUpdate();
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
    else if (!strcmp(id, "NoteEvents")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("Events", data, size);

        const auto* events = reinterpret_cast<
            const std::pair<uint32_t, float>*>(data);
        uint32 numEvents = size / sizeof(events[0]);

        // this is a synchronous send, because the update object gets reused
        noteUpdate_->setEvents(events, numEvents, false);
        noteUpdate_->changed();
        noteUpdate_->clear();
    }
    else if (!strcmp(id, "Automate")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("Data", data, size);

        if (result != kResultTrue)
            return result;

        const uint8* pos = reinterpret_cast<const uint8*>(data);
        const uint8* end = pos + size;

        while (static_cast<size_t>(end - pos) >= sizeof(uint32) + sizeof(float)) {
            Vst::ParamID pid = *reinterpret_cast<const uint32*>(pos);
            pos += sizeof(uint32);
            float value = *reinterpret_cast<const float*>(pos);
            pos += sizeof(float);
            setParam(pid, value);
        }
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
    continuousUpdates.push_back(sfzUpdate_);
    continuousUpdates.push_back(sfzDescriptionUpdate_);
    continuousUpdates.push_back(scalaUpdate_);
    continuousUpdates.push_back(playStateUpdate_);
    for (uint32 i = 0, n = parameters.getParameterCount(); i < n; ++i)
        continuousUpdates.push_back(parameters.getParameterByIndex(i));

    std::vector<FObject*> triggerUpdates;
    triggerUpdates.push_back(oscUpdate_);
    triggerUpdates.push_back(noteUpdate_);

    IPtr<SfizzVstEditor> editor = Steinberg::owned(
        new SfizzVstEditor(this, absl::MakeSpan(continuousUpdates), absl::MakeSpan(triggerUpdates)));

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
