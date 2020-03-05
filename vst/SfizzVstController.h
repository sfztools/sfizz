// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstState.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"
class SfizzVstState;

using namespace Steinberg;
using namespace VSTGUI;

class SfizzVstControllerNoUi : public Vst::EditController,
                               public Vst::IMidiMapping {
public:
    virtual ~SfizzVstControllerNoUi() {}

    tresult PLUGIN_API initialize(FUnknown* context) override;
    tresult PLUGIN_API terminate() override;

    tresult PLUGIN_API getMidiControllerAssignment(int32 busIndex, int16 channel, Vst::CtrlNumber midiControllerNumber, Vst::ParamID& id) override;

    enum { numControllerParams = 128 };

    // interfaces
    OBJ_METHODS(SfizzVstControllerNoUi, Vst::EditController)
    DEFINE_INTERFACES
    DEF_INTERFACE(Vst::IMidiMapping)
    END_DEFINE_INTERFACES(Vst::EditController)
    REFCOUNT_METHODS(Vst::EditController)

    enum {
        kPidMidiCC0,
        kPidMidiCCLast = kPidMidiCC0 + numControllerParams - 1,
        kPidMidiAftertouch,
        kPidMidiPitchBend,
        /* Reserved */
    };
};

class SfizzVstController : public SfizzVstControllerNoUi, public VSTGUI::VST3EditorDelegate {
public:
    IPlugView* PLUGIN_API createView(FIDString name) override;

    tresult PLUGIN_API setParamNormalized(Vst::ParamID tag, Vst::ParamValue value) override;
    tresult PLUGIN_API setComponentState(IBStream* state) override;

    struct StateListener {
        virtual void onStateChanged() = 0;
    };

    const SfizzVstState& getSfizzState() const { return _state; }

    void addStateListener(StateListener* listener);
    void removeStateListener(StateListener* listener);

    ///
    static FUnknown* createInstance(void*);

    static FUID cid;

private:
    SfizzVstState _state;
    std::vector<StateListener*> _stateListeners;
};
