// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstController.h"
#include "SfizzVstEditor.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

tresult PLUGIN_API SfizzVstControllerNoUi::initialize(FUnknown* context)
{
    tresult result = EditController::initialize(context);
    if (result != kResultTrue)
        return result;

    Vst::ParamID pid = 0;

    // Ordinary parameters
    parameters.addParameter(
        kParamVolumeRange.createParameter(
            Steinberg::String("Volume"), pid++, Steinberg::String("dB"),
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamNumVoicesRange.createParameter(
            Steinberg::String("Polyphony"), pid++, nullptr,
            0, Vst::ParameterInfo::kNoFlags, Vst::kRootUnitId));

    // MIDI controllers
    for (unsigned i = 0; i < kNumControllerParams; ++i) {
        Steinberg::String title;
        Steinberg::String shortTitle;
        title.printf("Controller %u", i);
        shortTitle.printf("CC%u", i);

        parameters.addParameter(
            title, nullptr, 0, 0, Vst::ParameterInfo::kCanAutomate,
            pid++, Vst::kRootUnitId, shortTitle);
    }

    // MIDI extra controllers
    parameters.addParameter(Steinberg::String("Aftertouch"), nullptr, 0, 0.5, 0, pid++, Vst::kRootUnitId);
    parameters.addParameter(Steinberg::String("Pitch Bend"), nullptr, 0, 0.5, 0, pid++, Vst::kRootUnitId);

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

// --- Controller with UI --- //

IPlugView* PLUGIN_API SfizzVstController::createView(FIDString _name)
{
    ConstString name(_name);

    if (name != Vst::ViewType::kEditor)
        return nullptr;

    return new SfizzVstEditor(this);
}

tresult PLUGIN_API SfizzVstController::setParamNormalized(Vst::ParamID tag, Vst::ParamValue normValue)
{
    tresult r = SfizzVstControllerNoUi::setParamNormalized(tag, normValue);
    if (r != kResultTrue)
        return r;

    float *slotF32 = nullptr;
    int32 *slotI32 = nullptr;
    float value = 0;

    switch (tag) {
    case kPidVolume: {
        slotF32 = &_state.volume;
        value = kParamVolumeRange.denormalize(normValue);
        break;
    }
    case kPidNumVoices: {
        slotI32 = &_state.numVoices;
        value = kParamNumVoicesRange.denormalize(normValue);
        break;
    }
    }

    bool update = false;

    if (slotF32 && *slotF32 != value) {
        *slotF32 = value;
        update = true;
    }
    else if (slotI32 && *slotI32 != (int32)value) {
        *slotI32 = (int32)value;
        update = true;
    }

    if (update) {
        for (StateListener* listener : _stateListeners)
            listener->onStateChanged();
    }

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstController::setState(IBStream* state)
{
    SfizzUiState s;

    tresult r = s.load(state);
    if (r != kResultTrue)
        return r;

    _uiState = s;

    for (StateListener* listener : _stateListeners)
        listener->onStateChanged();

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstController::getState(IBStream* state)
{
    return _uiState.store(state);
}

tresult PLUGIN_API SfizzVstController::setComponentState(IBStream* state)
{
    SfizzVstState s;

    tresult r = s.load(state);
    if (r != kResultTrue)
        return r;

    _state = s;

    setParamNormalized(kPidVolume, kParamVolumeRange.normalize(s.volume));
    setParamNormalized(kPidNumVoices, kParamNumVoicesRange.normalize(s.numVoices));

    for (StateListener* listener : _stateListeners)
        listener->onStateChanged();

    return kResultTrue;
}

void SfizzVstController::addSfizzStateListener(StateListener* listener)
{
    _stateListeners.push_back(listener);
}

void SfizzVstController::removeSfizzStateListener(StateListener* listener)
{
    auto it = std::find(_stateListeners.begin(), _stateListeners.end(), listener);
    if (it != _stateListeners.end())
        _stateListeners.erase(it);
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
