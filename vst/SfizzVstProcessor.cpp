// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstProcessor.h"
#include "SfizzVstController.h"
#include "SfizzVstState.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <cstring>

#pragma message("TODO: send tempo")

SfizzVstProcessor::SfizzVstProcessor()
    : _fifoToWorker(1024)
{
    setControllerClass(SfizzVstController::cid);
}

SfizzVstProcessor::~SfizzVstProcessor()
{
    setActive(false); // to be sure
}

tresult PLUGIN_API SfizzVstProcessor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultTrue)
        return result;

    addAudioOutput(STR16("Audio Output"), Vst::SpeakerArr::kStereo);
    addEventInput(STR16("Event Input"), 1);

    return result;
}

tresult PLUGIN_API SfizzVstProcessor::setBusArrangements(Vst::SpeakerArrangement* inputs, int32 numIns, Vst::SpeakerArrangement* outputs, int32 numOuts)
{
    bool isStereo = numIns == 0 && numOuts == 1 && outputs[0] == Vst::SpeakerArr::kStereo;

    if (!isStereo)
        return kResultFalse;

    return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
}

tresult PLUGIN_API SfizzVstProcessor::setState(IBStream* state)
{
    SfizzVstState s;

    tresult r = s.load(state);
    if (r != kResultTrue)
        return r;

    loadSfzFile(s.sfzFile);

    return r;
}

tresult PLUGIN_API SfizzVstProcessor::getState(IBStream* state)
{
    SfizzVstState s;
    {
        std::lock_guard<std::mutex> lock(_processMutex);
        s.sfzFile = _sfzFile;
    }

    return s.store(state);
}

tresult PLUGIN_API SfizzVstProcessor::canProcessSampleSize(int32 symbolicSampleSize)
{
    if (symbolicSampleSize != Vst::kSample32)
        return kResultFalse;

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstProcessor::setActive(TBool state)
{
    stopBackgroundWork();
    _synth.reset();

    if (state) {
        fprintf(stderr, "[Sfizz] new synth\n");
        sfz::Sfizz* synth = new sfz::Sfizz;
        _synth.reset(synth);

        synth->setSampleRate(processSetup.sampleRate);
        synth->setSamplesPerBlock(processSetup.maxSamplesPerBlock);

        loadSfzFile(_sfzFile);

        _workRunning = true;
        _worker = std::thread([this]() { doBackgroundWork(); });
    }

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstProcessor::process(Vst::ProcessData& data)
{
    sfz::Sfizz& synth = *_synth;

    if (data.numOutputs < 1)  // flush mode
        return kResultTrue;

    uint32 numFrames = data.numSamples;
    constexpr uint32 numChannels = 2;
    float* outputs[numChannels];

    assert(numChannels == data.outputs[0].numChannels);

    for (unsigned c = 0; c < numChannels; ++c)
        outputs[c] = data.outputs[0].channelBuffers32[c];

    std::unique_lock<std::mutex> lock(_processMutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        for (unsigned c = 0; c < numChannels; ++c)
            std::memset(outputs[c], 0, numFrames * sizeof(float));
        data.outputs[0].silenceFlags = 3;
        return kResultTrue;
    }

    if (Vst::IParameterChanges* pc = data.inputParameterChanges) {
        uint32 paramCount = pc->getParameterCount();

        for (uint32 paramIndex = 0; paramIndex < paramCount; ++paramIndex) {
            Vst::IParamValueQueue* vq = pc->getParameterData(paramIndex);

            Vst::ParamID id = vq->getParameterId();

            switch (id) {
            default:
                if (id >= SfizzVstController::kPidMidiCC0 && id <= SfizzVstController::kPidMidiCCLast) {
                    int ccNumber = id - SfizzVstController::kPidMidiCC0;
                    for (uint32 pointIndex = 0, pointCount = vq->getPointCount(); pointIndex < pointCount; ++pointIndex) {
                        int32 sampleOffset;
                        Vst::ParamValue value;
                        if (vq->getPoint(pointIndex, sampleOffset, value) == kResultTrue)
                            synth.cc(sampleOffset, ccNumber, (int)(0.5 + value * 127.0));
                    }
                }
                break;

            case SfizzVstController::kPidMidiAftertouch:
                for (uint32 pointIndex = 0, pointCount = vq->getPointCount(); pointIndex < pointCount; ++pointIndex) {
                    int32 sampleOffset;
                    Vst::ParamValue value;
                    if (vq->getPoint(pointIndex, sampleOffset, value) == kResultTrue)
                        synth.aftertouch(sampleOffset, (int)(0.5 + value * 127.0));
                }
                break;

            case SfizzVstController::kPidMidiPitchBend:
                for (uint32 pointIndex = 0, pointCount = vq->getPointCount(); pointIndex < pointCount; ++pointIndex) {
                    int32 sampleOffset;
                    Vst::ParamValue value;
                    if (vq->getPoint(pointIndex, sampleOffset, value) == kResultTrue)
                        synth.pitchWheel(sampleOffset, (int)(0.5 + value * 16383) - 8192);
                }
                break;
            }
        }
    }

    if (Vst::IEventList* events = data.inputEvents) {
        uint32 numEvents = events->getEventCount();

        for (uint32 i = 0; i < numEvents; i++) {
            Vst::Event e;
            if (events->getEvent(i, e) != kResultTrue)
                continue;

            auto convertVelocityFromFloat = [](float x) -> int {
                return std::min(127, std::max(0, (int)(x * 127.0f)));
            };

            switch (e.type) {
            case Vst::Event::kNoteOnEvent:
                synth.noteOn(e.sampleOffset, e.noteOn.pitch, convertVelocityFromFloat(e.noteOn.velocity));
                break;
            case Vst::Event::kNoteOffEvent:
                synth.noteOff(e.sampleOffset, e.noteOff.pitch, convertVelocityFromFloat(e.noteOff.velocity));
                break;
            // case Vst::Event::kPolyPressureEvent:
            //     synth.aftertouch(e.sampleOffset, convertVelocityFromFloat(e.polyPressure.pressure));
            //     break;
            }
        }
    }

    synth.renderBlock(outputs, numFrames, numChannels);
    return kResultTrue;
}

tresult PLUGIN_API SfizzVstProcessor::notify(Vst::IMessage* message)
{
    tresult result = AudioEffect::notify(message);
    if (result != kResultFalse)
        return result;

    if (!_fifoToWorker.push(message))
        return kOutOfMemory;

    message->addRef();
    _semaToWorker.post();

    return kResultTrue;
}

FUnknown* SfizzVstProcessor::createInstance(void*)
{
    return static_cast<Vst::IAudioProcessor*>(new SfizzVstProcessor);
}

void SfizzVstProcessor::loadSfzFile(std::string file)
{
    std::lock_guard<std::mutex> lock(_processMutex);

    if (_synth) {
        fprintf(stderr, "[Sfizz] load SFZ file: %s\n", file.c_str());
        _synth->loadSfzFile(file);
    }

    _sfzFile = std::move(file);
}

void SfizzVstProcessor::doBackgroundWork()
{
    constexpr uint32 maxPathLen = 32768;

    for (;;) {
        _semaToWorker.wait();

        if (!_workRunning)
            break;

        Vst::IMessage* msg;
        if (!_fifoToWorker.pop(msg)) {
            fprintf(stderr, "[Sfizz] message synchronization error in worker\n");
            std::abort();
        }

        const char* id = msg->getMessageID();
        Vst::IAttributeList* attr = msg->getAttributes();

        if (!std::strcmp(id, "LoadSfz")) {
            std::vector<Vst::TChar> path(maxPathLen + 1);
            if (attr->getString("File", path.data(), maxPathLen) == kResultTrue)
                loadSfzFile(Steinberg::String(path.data()).text8());
        }

        msg->release();
    }
}

void SfizzVstProcessor::stopBackgroundWork()
{
    if (!_workRunning)
        return;

    _workRunning = false;
    _semaToWorker.post();
    _worker.join();

    while (_semaToWorker.try_wait()) {
        Vst::IMessage* msg;
        if (!_fifoToWorker.pop(msg)) {
            fprintf(stderr, "[Sfizz] message synchronization error in processor\n");
            std::abort();
        }
        msg->release();
    }
}

/*
  Note(jpc) Generated at random with uuidgen.
  Can't find docs on it... maybe it's to register somewhere?
 */
FUID SfizzVstProcessor::cid(0xe8fab718, 0x15ed46e3, 0x8b598310, 0x1e12993f);
