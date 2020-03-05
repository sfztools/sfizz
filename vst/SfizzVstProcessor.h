// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstState.h"
#include "RTSemaphore.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/utility/ringbuffer.h"
#include <sfizz.hpp>
#include <thread>
#include <mutex>
#include <memory>

using namespace Steinberg;

class SfizzVstProcessor : public Vst::AudioEffect {
public:
    SfizzVstProcessor();
    ~SfizzVstProcessor();

    tresult PLUGIN_API initialize(FUnknown* context) override;
    tresult PLUGIN_API setBusArrangements(Vst::SpeakerArrangement* inputs, int32 numIns, Vst::SpeakerArrangement* outputs, int32 numOuts) override;

    tresult PLUGIN_API setState(IBStream* stream) override;
    tresult PLUGIN_API getState(IBStream* stream) override;
    void syncStateToSynth();

    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override;
    tresult PLUGIN_API setActive(TBool state) override;
    tresult PLUGIN_API process(Vst::ProcessData& data) override;
    void processParameterChanges(Vst::IParameterChanges& pc);
    void processControllerChanges(Vst::IParameterChanges& pc);
    void processEvents(Vst::IEventList& events);
    static int convertVelocityFromFloat(float x);

    tresult PLUGIN_API notify(Vst::IMessage* message) override;

    static FUnknown* createInstance(void*);

    static FUID cid;

    // --- Sfizz stuff here below ---
private:
    // synth state. acquire processMutex before accessing
    std::unique_ptr<sfz::Sfizz> _synth;
    SfizzVstState _state;

    // worker and thread sync
    std::thread _worker;
    volatile bool _workRunning = false;
    Steinberg::OneReaderOneWriter::RingBuffer<Vst::IMessage*> _fifoToWorker;
    RTSemaphore _semaToWorker;
    std::mutex _processMutex;

    // worker
    void doBackgroundWork();
    void stopBackgroundWork();
};
