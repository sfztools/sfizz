// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstIDs.h"
#include "public.sdk/source/vst/vst2wrapper/vst2wrapper.h"

//------------------------------------------------------------------------
::AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
    return Steinberg::Vst::Vst2Wrapper::create(
        GetPluginFactory(),
        SfizzVstProcessor_cid,
        'Sfzz',
        audioMaster);
}
