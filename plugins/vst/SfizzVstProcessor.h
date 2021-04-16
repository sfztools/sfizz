// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstState.h"
#include "OrderedEventProcessor.h"
#include "sfizz/RTSemaphore.h"
#include "ring_buffer/ring_buffer.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include <sfizz.hpp>
#include <SpinMutex.h>
#include <array>
#include <thread>
#include <memory>
#include <cstdlib>

using namespace Steinberg;

class SfizzVstProcessor : public Vst::AudioEffect,
                          public OrderedEventProcessor<SfizzVstProcessor> {
public:
    SfizzVstProcessor();
    ~SfizzVstProcessor();

    tresult PLUGIN_API initialize(FUnknown* context) override;
    tresult PLUGIN_API setBusArrangements(Vst::SpeakerArrangement* inputs, int32 numIns, Vst::SpeakerArrangement* outputs, int32 numOuts) override;

    tresult PLUGIN_API connect(IConnectionPoint* other) override;

    tresult PLUGIN_API setState(IBStream* stream) override;
    tresult PLUGIN_API getState(IBStream* stream) override;
    void syncStateToSynth();

    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override;
    tresult PLUGIN_API setActive(TBool state) override;
    tresult PLUGIN_API process(Vst::ProcessData& data) override;

    // OrderedEventProcessor
    void playOrderedParameter(int32 sampleOffset, Vst::ParamID id, Vst::ParamValue value);
    void playOrderedEvent(const Vst::Event& event);

    void processMessagesFromUi();

    tresult PLUGIN_API notify(Vst::IMessage* message) override;

    static FUnknown* createInstance(void*);

    static FUID cid;

    // --- Sfizz stuff here below ---
private:
    // synth state. acquire processMutex before accessing
    std::unique_ptr<sfz::Sfizz> _synth;
    Steinberg::IPtr<Vst::IMessage> _loadedSfzMessage;
    Steinberg::IPtr<Vst::IMessage> _automateMessage;
    bool _isActive = false;
    SfizzVstState _state;
    float _currentStretchedTuning = 0;

    // whether allowed to perform events (owns the processing lock)
    bool _canPerformEventsAndParameters {};

    // client
    sfz::ClientPtr _client;
    std::unique_ptr<uint8_t[]> _oscTemp;
    void receiveMessage(int delay, const char* path, const char* sig, const sfizz_arg_t* args);

    // misc
    void loadSfzFileOrDefault(const std::string& filePath, bool initParametersFromState);

    // note event tracking
    std::array<float, 128> _noteEventsCurrentCycle; // 0: off, >0: on, <0: no change

    // worker and thread sync
    std::thread _worker;
    volatile bool _workRunning = false;
    Ring_Buffer _fifoToWorker;
    RTSemaphore _semaToWorker;
    Ring_Buffer _fifoMessageFromUi;
    SpinMutex _processMutex;

    // time info
    int _timeSigNumerator = 0;
    int _timeSigDenominator = 0;
    void updateTimeInfo(const Vst::ProcessContext& context);

    // messaging
    struct RTMessage {
        const char* type;
        uintptr_t size;
        // 32-bit aligned data after header
        template <class T> const T* payload() const;
    };
    struct RTMessageDelete {
        void operator()(RTMessage* x) const noexcept { std::free(x); }
    };
    typedef std::unique_ptr<RTMessage, RTMessageDelete> RTMessagePtr;

    // worker
    void doBackgroundWork();
    void doBackgroundIdle(size_t idleCounter);
    void startBackgroundWork();
    void stopBackgroundWork();
    // writer
    bool writeWorkerMessage(const char* type, const void* data, uintptr_t size);
    // reader
    RTMessagePtr readWorkerMessage();
    bool discardWorkerMessage();

    // generic
    static bool writeMessage(Ring_Buffer& fifo, const char* type, const void* data, uintptr_t size);
};

//------------------------------------------------------------------------------

template <class T> const T* SfizzVstProcessor::RTMessage::payload() const
{
    return reinterpret_cast<const T*>(
        reinterpret_cast<const uint8*>(this) + sizeof(*this));
}
