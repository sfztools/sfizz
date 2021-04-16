// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstIDs.h"
#include "VstPluginDefs.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"

class SfizzVstProcessor;
class SfizzVstController;

BEGIN_FACTORY_DEF(VSTPLUGIN_VENDOR,
                  VSTPLUGIN_URL,
                  "mailto:" VSTPLUGIN_EMAIL)

DEF_CLASS2 (INLINE_UID_FROM_FUID(SfizzVstProcessor_cid),
            PClassInfo::kManyInstances,
            kVstAudioEffectClass,
            VSTPLUGIN_NAME,
            Vst::kDistributable,
            Vst::PlugType::kInstrumentSynth,
            VSTPLUGIN_VERSION,
            kVstVersionString,
            createInstance<SfizzVstProcessor>)

DEF_CLASS2 (INLINE_UID_FROM_FUID(SfizzVstController_cid),
            PClassInfo::kManyInstances,
            kVstComponentControllerClass,
            VSTPLUGIN_NAME,
            0,						// not used here
            "",						// not used here
            VSTPLUGIN_VERSION,
            kVstVersionString,
            createInstance<SfizzVstController>)

END_FACTORY
