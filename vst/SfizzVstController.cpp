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

    // MIDI controllers
    for (unsigned i = 0; i < numControllerParams; ++i) {
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
        if (midiControllerNumber < 0 || midiControllerNumber >= numControllerParams)
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

tresult PLUGIN_API SfizzVstController::setParamNormalized(Vst::ParamID tag, Vst::ParamValue value)
{
    tresult r = SfizzVstControllerNoUi::setParamNormalized(tag, value);
    if (r != kResultTrue)
        return r;

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstController::setComponentState(IBStream* state)
{
    SfizzVstState s;

    tresult r = s.load(state);
    if (r != kResultTrue)
        return r;

    for (StateListener* listener : _stateListeners)
        listener->onStateChanged();

    _state = s;
    return kResultTrue;
}

void SfizzVstController::addStateListener(StateListener* listener)
{
    _stateListeners.push_back(listener);
}

void SfizzVstController::removeStateListener(StateListener* listener)
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
