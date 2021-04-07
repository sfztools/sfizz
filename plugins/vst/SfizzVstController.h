// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstState.h"
#include "SfizzVstUpdates.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include <sfizz_message.h>
#include <mutex>
class SfizzVstState;
class SfizzVstEditor;

using namespace Steinberg;
using namespace VSTGUI;

class SfizzVstControllerNoUi : public Vst::EditController,
                               public Vst::IMidiMapping {
public:
    virtual ~SfizzVstControllerNoUi() {}

    tresult PLUGIN_API initialize(FUnknown* context) override;
    tresult PLUGIN_API terminate() override;

    tresult PLUGIN_API getMidiControllerAssignment(int32 busIndex, int16 channel, Vst::CtrlNumber midiControllerNumber, Vst::ParamID& id) override;

    tresult PLUGIN_API getParamStringByValue(Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string) override;
    tresult PLUGIN_API getParamValueByString(Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized) override;

    tresult setParam(Vst::ParamID tag, float value);
    tresult PLUGIN_API setComponentState(IBStream* stream) override;
    tresult PLUGIN_API notify(Vst::IMessage* message) override;

    // interfaces
    OBJ_METHODS(SfizzVstControllerNoUi, Vst::EditController)
    DEFINE_INTERFACES
    DEF_INTERFACE(Vst::IMidiMapping)
    END_DEFINE_INTERFACES(Vst::EditController)
    REFCOUNT_METHODS(Vst::EditController)

protected:
    Steinberg::IPtr<OSCUpdate> oscUpdate_;
    Steinberg::IPtr<NoteUpdate> noteUpdate_;
    Steinberg::IPtr<SfzUpdate> sfzUpdate_;
    Steinberg::IPtr<SfzDescriptionUpdate> sfzDescriptionUpdate_;
    Steinberg::IPtr<ScalaUpdate> scalaUpdate_;
    Steinberg::IPtr<PlayStateUpdate> playStateUpdate_;
    Vst::ParamID midiMapping_[Vst::kCountCtrlNumber] {};
};

class SfizzVstController : public SfizzVstControllerNoUi, public VSTGUI::VST3EditorDelegate {
public:
    IPlugView* PLUGIN_API createView(FIDString name) override;

    static FUnknown* createInstance(void*);

    static FUID cid;
};
