// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstProcessor.h"
#include "SfizzVstController.h"
#include "SfizzVstState.h"
#include "SfizzVstParameters.h"
#include "SfizzVstIDs.h"
#include "sfizz/import/sfizz_import.h"
#include "plugin/SfizzFileScan.h"
#include "plugin/InstrumentDescription.h"
#include "base/source/fstreamer.h"
#include "base/source/updatehandler.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <ghc/fs_std.hpp>
#include <chrono>
#include <cstring>

static const char defaultSfzText[] =
    "<region>sample=*sine" "\n"
    "ampeg_attack=0.02 ampeg_release=0.1" "\n";

enum {
    kMidiEventMaximumSize = 4,
    kOscTempSize = 8192,
};

static const char* kRingIdMidi = "Mid";
static const char* kRingIdOsc = "Osc";

static const char* kMsgIdSetNumVoices = "SetNumVoices";
static const char* kMsgIdSetOversampling = "SetOversampling";
static const char* kMsgIdSetPreloadSize = "SetPreloadSize";
static const char* kMsgIdReceiveOSC = "ReceiveOSC";
static const char* kMsgIdNoteEvents = "NoteEvents";

static constexpr std::chrono::milliseconds kBackgroundIdleInterval { 20 };

SfizzVstProcessor::SfizzVstProcessor()
    : _oscTemp(new uint8_t[kOscTempSize]),
      _fifoToWorker(64 * 1024), _fifoMessageFromUi(64 * 1024)
{
    setControllerClass(SfizzVstController::cid);

    // ensure the SFZ path exists:
    // the one specified in the configuration, otherwise the fallback
    absl::optional<fs::path> configDefaultPath = SfizzPaths::getSfzConfigDefaultPath();
    if (configDefaultPath) {
        std::error_code ec;
        fs::create_directory(*configDefaultPath, ec);
    }
    else {
        fs::path fallbackDefaultPath = SfizzPaths::getSfzFallbackDefaultPath();
        std::error_code ec;
        fs::create_directory(fallbackDefaultPath, ec);
    }
}

SfizzVstProcessor::~SfizzVstProcessor()
{
    try {
        stopBackgroundWork();
    } catch (const std::exception& e) {
        fprintf(stderr, "Caught exception: %s\n", e.what());
    }
}

tresult PLUGIN_API SfizzVstProcessor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultTrue)
        return result;

    // initialize the update handler
    Steinberg::UpdateHandler::instance();

    _queuedMessages = Steinberg::owned(new QueuedUpdates);
    _playStateUpdate = Steinberg::owned(new PlayStateUpdate);
    _sfzUpdate = Steinberg::owned(new SfzUpdate);
    _sfzDescriptionUpdate = Steinberg::owned(new SfzDescriptionUpdate);
    _scalaUpdate = Steinberg::owned(new ScalaUpdate);
    _automationUpdate = Steinberg::owned(new AutomationUpdate);

    _queuedMessages->addDependent(this);
    _playStateUpdate->addDependent(this);
    _sfzUpdate->addDependent(this);
    _sfzDescriptionUpdate->addDependent(this);
    _scalaUpdate->addDependent(this);
    _automationUpdate->addDependent(this);

    addAudioOutput(STR16("Audio Output 1"), Vst::SpeakerArr::kStereo);
    addEventInput(STR16("Event Input"), 1);

    _state = SfizzVstState();

    // allocate needed space to track CC values
    _state.controllers.resize(sfz::config::numCCs);

    fprintf(stderr, "[sfizz] new synth\n");
    _synth.reset(new sfz::Sfizz);

    auto onMessage = +[](void* data, int delay, const char* path, const char* sig, const sfizz_arg_t* args)
        {
            auto *self = reinterpret_cast<SfizzVstProcessor*>(data);
            self->receiveOSC(delay, path, sig, args);
        };
    _client = _synth->createClient(this);
    _synth->setReceiveCallback(*_client, onMessage);
    _synth->setBroadcastCallback(onMessage, this);

    _currentStretchedTuning = 0.0;
    loadSfzFileOrDefault({}, false);

    _synth->bpmTempo(0, 120);
    _timeSigNumerator = 4;
    _timeSigDenominator = 4;
    _synth->timeSignature(0, _timeSigNumerator, _timeSigDenominator);
    _synth->timePosition(0, 0, 0);
    _synth->playbackState(0, 0);

    _noteEventsCurrentCycle.fill(-1.0f);

    _editorIsOpen = false;

    return result;
}

tresult PLUGIN_API SfizzVstProcessor::terminate()
{
    _queuedMessages->removeDependent(this);
    _playStateUpdate->removeDependent(this);
    _sfzUpdate->removeDependent(this);
    _sfzDescriptionUpdate->removeDependent(this);
    _scalaUpdate->removeDependent(this);
    _automationUpdate->removeDependent(this);

    return AudioEffect::terminate();
}

tresult PLUGIN_API SfizzVstProcessor::setBusArrangements(Vst::SpeakerArrangement* inputs, int32 numIns, Vst::SpeakerArrangement* outputs, int32 numOuts)
{
    bool allStereo { true };
    for (unsigned o = 0; o < numOuts; ++o)
        allStereo &= (outputs[o] == Vst::SpeakerArr::kStereo);

    if (!allStereo)
        return kResultFalse;

    return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
}

tresult PLUGIN_API SfizzVstProcessor::connect(IConnectionPoint* other)
{
    tresult result = AudioEffect::connect(other);
    if (result != kResultTrue)
        return result;

    // when controller connects, send these messages that we couldn't earlier
    _queuedMessages->deferUpdate();

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstProcessor::setState(IBStream* stream)
{
    SfizzVstState s;

    tresult r = s.load(stream);
    if (r != kResultTrue)
        return r;

    // check the files to really exist, otherwise search them
    for (std::string* statePath : { &s.sfzFile, &s.scalaFile }) {
        if (statePath->empty())
            continue;

        fs::path pathOrig = fs::u8path(*statePath);
        std::error_code ec;
        if (fs::is_regular_file(pathOrig, ec))
            continue;

        fprintf(stderr, "[Sfizz] searching for missing file: %s\n", pathOrig.filename().u8string().c_str());

        SfzFileScan& fileScan = SfzFileScan::getInstance();
        fs::path pathFound;
        if (!fileScan.locateRealFile(pathOrig, pathFound))
            fprintf(stderr, "[Sfizz] file not found: %s\n", pathOrig.filename().u8string().c_str());
        else {
            fprintf(stderr, "[Sfizz] file found: %s\n", pathFound.u8string().c_str());
            *statePath = pathFound.u8string();
        }
    }

    //
    std::lock_guard<SpinMutex> lock(_processMutex);
    _state = s;

    // allocate needed space to track CC values
    _state.controllers.resize(sfz::config::numCCs);

    syncStateToSynth();

    return r;
}

tresult PLUGIN_API SfizzVstProcessor::getState(IBStream* stream)
{
    std::lock_guard<SpinMutex> lock(_processMutex);
    return _state.store(stream);
}

void SfizzVstProcessor::syncStateToSynth()
{
    sfz::Sfizz* synth = _synth.get();

    if (!synth)
        return;

    loadSfzFileOrDefault(_state.sfzFile, true);
    synth->setVolume(_state.volume);
    synth->setNumVoices(_state.numVoices);
    synth->setOversamplingFactor(1 << _state.oversamplingLog2);
    synth->setPreloadSize(_state.preloadSize);
    synth->loadScalaFile(_state.scalaFile);
    synth->setScalaRootKey(_state.scalaRootKey);
    synth->setTuningFrequency(_state.tuningFrequency);
    synth->loadStretchTuningByRatio(_state.stretchedTuning);
    if (_state.lastKeyswitch >= 0 && _state.lastKeyswitch <= 127) {
        synth->hdNoteOn(0, _state.lastKeyswitch, 1.0f);
        synth->hdNoteOff(1, _state.lastKeyswitch, 0.0f);
    }
}

tresult PLUGIN_API SfizzVstProcessor::canProcessSampleSize(int32 symbolicSampleSize)
{
    if (symbolicSampleSize != Vst::kSample32)
        return kResultFalse;

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstProcessor::setActive(TBool state)
{
    sfz::Sfizz* synth = _synth.get();

    if (bool(state) == _isActive)
        return kResultTrue;

    if (!synth)
        return kResultFalse;

    if (state) {
        synth->setSampleRate(processSetup.sampleRate);
        synth->setSamplesPerBlock(processSetup.maxSamplesPerBlock);
        _rmsFollower.init(processSetup.sampleRate);
        initializeEventProcessor(processSetup, kNumParameters);
        startBackgroundWork();
    } else {
        stopBackgroundWork();
        synth->allSoundOff();
    }

    _isActive = bool(state);
    return kResultTrue;
}

tresult PLUGIN_API SfizzVstProcessor::process(Vst::ProcessData& data)
{
    sfz::Sfizz& synth = *_synth;

    std::unique_lock<SpinMutex> lock(_processMutex, std::defer_lock);
    if (data.processMode == Vst::kOffline)
        lock.lock();
    else
        (void)lock.try_lock();

    if (data.processContext)
        updateTimeInfo(*data.processContext);

    const uint32 numFrames = data.numSamples;
    _canPerformEventsAndParameters = lock.owns_lock();
    bool editorWasOpen = _editorIsOpen;
    processUnorderedEvents(numFrames, data.inputParameterChanges, data.inputEvents);

    if (data.numOutputs < 1)  // flush mode
        return kResultTrue;

    constexpr uint32 numChannels = 2;
    constexpr uint32 maxChannels = 16;
    float* outputs[maxChannels];
    const auto numMonoChannels = data.numOutputs * numChannels;

    for (unsigned o = 0; o < data.numOutputs; ++o) {
        assert(data.outputs[o].numChannels == numChannels);
        for (unsigned c = 0; c < numChannels; ++c)
            outputs[numChannels * o + c] = data.outputs[o].channelBuffers32[c];
    }

    if (!editorWasOpen && _editorIsOpen) {
        if (Vst::IParameterChanges* pcs = data.outputParameterChanges) {
            int32 index;
            if (Vst::IParamValueQueue* vq = pcs->addParameterData(kPidNumOutputs, index))
                vq->addPoint(0, SfizzRange::getForParameter(kPidNumOutputs).normalize(numMonoChannels), index);
        }
    }

    if (!lock.owns_lock()) {
        for (unsigned c = 0; c < numChannels; ++c)
            std::memset(outputs[c], 0, numFrames * sizeof(float));

        for (unsigned o = 0; o < data.numOutputs; ++o)
            data.outputs[o].silenceFlags = 3;

        return kResultTrue;
    }

    if (data.processMode == Vst::kOffline)
        synth.enableFreeWheeling();
    else
        synth.disableFreeWheeling();

    processMessagesFromUi();

    synth.setVolume(_state.volume);
    synth.setScalaRootKey(_state.scalaRootKey);
    synth.setTuningFrequency(_state.tuningFrequency);
    if (_currentStretchedTuning != _state.stretchedTuning) {
        synth.loadStretchTuningByRatio(_state.stretchedTuning);
        _currentStretchedTuning = _state.stretchedTuning;
    }
    synth.setSampleQuality(sfz::Sfizz::ProcessLive, _state.sampleQuality);
    synth.setOscillatorQuality(sfz::Sfizz::ProcessLive, _state.oscillatorQuality);
    synth.setSampleQuality(sfz::Sfizz::ProcessFreewheeling, _state.freewheelingSampleQuality);
    synth.setOscillatorQuality(sfz::Sfizz::ProcessFreewheeling, _state.freewheelingOscillatorQuality);
    synth.setSustainCancelsRelease(_state.sustainCancelsRelease);

    synth.renderBlock(outputs, numFrames, data.numOutputs);

    // Update levels, if editor is open, otherwise skip
    RMSFollower& rmsFollower = _rmsFollower;
    if (_editorIsOpen) {
        rmsFollower.process((const float**)outputs, numFrames, numMonoChannels);
        float levels[maxChannels];
        rmsFollower.getRMS(levels, numMonoChannels);
        if (Vst::IParameterChanges* pcs = data.outputParameterChanges) {
            int32 index;
            for (int c = 0; c < numMonoChannels; ++c) {
                if (Vst::IParamValueQueue* vq = pcs->addParameterData(kPidLevel0 + c, index))
                    vq->addPoint(0, levels[c], index);
            }
        }
    }
    else
        rmsFollower.clear();

    // Request OSC updates
    sfz::Client& client = *_client;
    synth.sendMessage(client, 0, "/sw/last/current", "", nullptr);

    //
    NoteUpdate::Item noteEvents[128];
    size_t numNoteEvents = 0;
    for (uint32 key = 0; key < 128; ++key) {
        float value = _noteEventsCurrentCycle[key];
        if (value < 0.0f)
            continue;
        noteEvents[numNoteEvents++] = std::make_pair(key, value);
        _noteEventsCurrentCycle[key] = -1.0f;
    }
    if (numNoteEvents > 0) {
        if (writeWorkerMessage(kMsgIdNoteEvents, noteEvents, numNoteEvents * sizeof(noteEvents[0])))
            _semaToWorker.post();
    }

    return kResultTrue;
}

void SfizzVstProcessor::updateTimeInfo(const Vst::ProcessContext& context)
{
    sfz::Sfizz& synth = *_synth;

    if (context.state & context.kTempoValid)
        synth.bpmTempo(0, static_cast<float>(context.tempo));

    if (context.state & context.kTimeSigValid) {
        _timeSigNumerator = context.timeSigNumerator;
        _timeSigDenominator = context.timeSigDenominator;
        synth.timeSignature(0, _timeSigNumerator, _timeSigDenominator);
    }

    if (context.state & context.kProjectTimeMusicValid) {
        double beats = context.projectTimeMusic * 0.25 * _timeSigDenominator;
        double bars = beats / _timeSigNumerator;
        beats -= int(bars) * _timeSigNumerator;
        synth.timePosition(0, int(bars), beats);
    }

    synth.playbackState(0, (context.state & context.kPlaying) != 0);
}

void SfizzVstProcessor::playOrderedParameter(int32 sampleOffset, Vst::ParamID id, Vst::ParamValue value)
{
    if (!_canPerformEventsAndParameters)
        return;

    sfz::Sfizz& synth = *_synth;
    const SfizzRange range = SfizzRange::getForParameter(id);

    switch (id) {
    case kPidVolume:
        _state.volume = range.denormalize(value);
        break;
    case kPidNumVoices:
        {
            int32 data = static_cast<int32>(range.denormalize(value));
            _state.numVoices = data;
            if (writeWorkerMessage(kMsgIdSetNumVoices, &data, sizeof(data)))
                _semaToWorker.post();
        }
        break;
    case kPidOversampling:
        {
            int32 data = static_cast<int32>(range.denormalize(value));
            _state.oversamplingLog2 = data;
            if (writeWorkerMessage(kMsgIdSetOversampling, &data, sizeof(data)))
                _semaToWorker.post();
        }
        break;
    case kPidPreloadSize:
        {
            int32 data = static_cast<int32>(range.denormalize(value));
            _state.preloadSize = data;
            if (writeWorkerMessage(kMsgIdSetPreloadSize, &data, sizeof(data)))
                _semaToWorker.post();
        }
        break;
    case kPidScalaRootKey:
        _state.scalaRootKey = static_cast<int32>(range.denormalize(value));
        break;
    case kPidTuningFrequency:
        _state.tuningFrequency = range.denormalize(value);
        break;
    case kPidStretchedTuning:
        _state.stretchedTuning = range.denormalize(value);
        break;
    case kPidSampleQuality:
        _state.sampleQuality = static_cast<int32>(range.denormalize(value));
        break;
    case kPidOscillatorQuality:
        _state.oscillatorQuality = static_cast<int32>(range.denormalize(value));
        break;
    case kPidFreewheelingSampleQuality:
        _state.freewheelingSampleQuality = static_cast<int32>(range.denormalize(value));
        break;
    case kPidFreewheelingOscillatorQuality:
        _state.freewheelingOscillatorQuality = static_cast<int32>(range.denormalize(value));
        break;
    case kPidSustainCancelsRelease:
        _state.sustainCancelsRelease = (range.denormalize(value) > 0.0f);
        break;
    case kPidAftertouch:
        synth.hdChannelAftertouch(sampleOffset, value);
        break;
    case kPidPitchBend:
        synth.hdPitchWheel(sampleOffset, range.denormalize(value));
        break;
    case kPidEditorOpen:
        _editorIsOpen = value != 0;
        break;
    default:
        if (id >= kPidCC0 && id <= kPidCCLast) {
            int32 ccNumber = static_cast<int32>(id - kPidCC0);
            synth.automateHdcc(sampleOffset, ccNumber, value);
            _state.controllers[ccNumber] = value;
        }
        break;
    }
}

void SfizzVstProcessor::playOrderedEvent(const Vst::Event& event)
{
    if (!_canPerformEventsAndParameters)
        return;

    sfz::Sfizz& synth = *_synth;
    const int32 sampleOffset = event.sampleOffset;

    switch (event.type) {
    case Vst::Event::kNoteOnEvent: {
        int pitch = event.noteOn.pitch;
        if (pitch < 0 || pitch >= 128)
            break;
        if (event.noteOn.velocity <= 0.0f) {
            synth.noteOff(sampleOffset, pitch, 0);
            _noteEventsCurrentCycle[pitch] = 0.0f;
        }
        else {
            synth.hdNoteOn(sampleOffset, pitch, event.noteOn.velocity);
            _noteEventsCurrentCycle[pitch] = event.noteOn.velocity;
        }
        break;
    }
    case Vst::Event::kNoteOffEvent: {
        int pitch = event.noteOn.pitch;
        if (pitch < 0 || pitch >= 128)
            break;
        synth.hdNoteOff(sampleOffset, pitch, event.noteOff.velocity);
        _noteEventsCurrentCycle[pitch] = 0.0f;
        break;
    }
    case Vst::Event::kPolyPressureEvent: {
        int pitch = event.polyPressure.pitch;
        if (pitch < 0 || pitch >= 128)
            break;
        synth.hdPolyAftertouch(sampleOffset, pitch, event.polyPressure.pressure);
        break;
    }
    }
}

void SfizzVstProcessor::processMessagesFromUi()
{
    sfz::Sfizz& synth = *_synth;
    sfz::Client& client = *_client;
    Ring_Buffer& fifo = _fifoMessageFromUi;
    RTMessage header;

    while (fifo.peek(header) && fifo.size_used() >= sizeof(header) + header.size) {
        fifo.discard(sizeof(header));

        if (header.type == kRingIdMidi) {
            if (header.size > kMidiEventMaximumSize) {
                fifo.discard(header.size);
                continue;
            }

            uint8_t data[kMidiEventMaximumSize] = {};
            fifo.get(data, header.size);

            // interpret the MIDI message
            switch (data[0] & 0xf0) {
            case 0x80:
                synth.noteOff(0, data[1] & 0x7f, data[2] & 0x7f);
                break;
            case 0x90:
                synth.noteOn(0, data[1] & 0x7f, data[2] & 0x7f);
                break;
            case 0xb0:
                synth.automateHdcc(0, data[1] & 0x7f, static_cast<float>(data[2] & 0x7f) / 127.0f);
                break;
            case 0xe0:
                synth.pitchWheel(0, (data[2] << 7) + data[1] - 8192);
                break;
            }
        }
        else if (header.type == kRingIdOsc) {
            uint8_t* oscTemp = _oscTemp.get();

            if (header.size > kOscTempSize) {
                fifo.discard(header.size);
                continue;
            }

            fifo.get(oscTemp, header.size);

            const char* path;
            const char* sig;
            const sfizz_arg_t* args;
            uint8_t buffer[1024];
            if (sfizz_extract_message(oscTemp, header.size, buffer, sizeof(buffer), &path, &sig, &args) > 0)
                synth.sendMessage(client, 0, path, sig, args);
        }
        else {
            assert(false);
            return;
        }
    }
}

tresult PLUGIN_API SfizzVstProcessor::notify(Vst::IMessage* message)
{
    // Note(jpc) this notification is not handled by the RT thread

    tresult result = AudioEffect::notify(message);
    if (result != kResultFalse)
        return result;

    const char* id = message->getMessageID();
    Vst::IAttributeList* attr = message->getAttributes();

    if (!std::strcmp(id, "LoadSfz")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("File", data, size);

        if (result != kResultTrue)
            return result;

        std::unique_lock<SpinMutex> lock(_processMutex);
        _state.sfzFile.assign(static_cast<const char *>(data), size);
        loadSfzFileOrDefault(_state.sfzFile, false);
        lock.unlock();
    }
    else if (!std::strcmp(id, "LoadScala")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("File", data, size);

        if (result != kResultTrue)
            return result;

        std::unique_lock<SpinMutex> lock(_processMutex);
        _state.scalaFile.assign(static_cast<const char *>(data), size);
        _synth->loadScalaFile(_state.scalaFile);
        lock.unlock();

        _scalaUpdate->setPath(_state.scalaFile);
        _scalaUpdate->deferUpdate();
    }
    else if (!std::strcmp(id, "MidiMessage")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("Data", data, size);
        if (size < kMidiEventMaximumSize)
            writeMessage(_fifoMessageFromUi, kRingIdMidi, data, size);
    }
    else if (!std::strcmp(id, "OscMessage")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("Data", data, size);
        writeMessage(_fifoMessageFromUi, kRingIdOsc, data, size);
    }

    return result;
}

void PLUGIN_API SfizzVstProcessor::update(FUnknown* changedUnknown, int32 message)
{
    if (processUpdate(changedUnknown, message))
        return;

    AudioEffect::update(changedUnknown, message);
}

bool SfizzVstProcessor::processUpdate(FUnknown* changedUnknown, int32 message)
{
    if (QueuedUpdates* update = FCast<QueuedUpdates>(changedUnknown)) {
        for (FObject* queuedUpdate : update->getUpdates(this))
            processUpdate(queuedUpdate, message);
        return true;
    }

    if (OSCUpdate* update = FCast<OSCUpdate>(changedUnknown)) {
        if (IPtr<Vst::IMessage> msg = update->convertToMessage(this))
            sendMessage(msg);
        return true;
    }

    if (PlayStateUpdate* update = FCast<PlayStateUpdate>(changedUnknown)) {
        if (IPtr<Vst::IMessage> msg = update->convertToMessage(this))
            sendMessage(msg);
        return true;
    }

    if (NoteUpdate* update = FCast<NoteUpdate>(changedUnknown)) {
        if (IPtr<Vst::IMessage> msg = update->convertToMessage(this))
            sendMessage(msg);
        return true;
    }

    if (SfzUpdate* update = FCast<SfzUpdate>(changedUnknown)) {
        if (IPtr<Vst::IMessage> msg = update->convertToMessage(this))
            sendMessage(msg);
        return true;
    }

    if (SfzDescriptionUpdate* update = FCast<SfzDescriptionUpdate>(changedUnknown)) {
        if (IPtr<Vst::IMessage> msg = update->convertToMessage(this))
            sendMessage(msg);
        return true;
    }

    if (ScalaUpdate* update = FCast<ScalaUpdate>(changedUnknown)) {
        if (IPtr<Vst::IMessage> msg = update->convertToMessage(this))
            sendMessage(msg);
        return true;
    }

    if (AutomationUpdate* update = FCast<AutomationUpdate>(changedUnknown)) {
        if (IPtr<Vst::IMessage> msg = update->convertToMessage(this))
            sendMessage(msg);
        return true;
    }

    return false;
}

void SfizzVstProcessor::receiveOSC(int delay, const char* path, const char* sig, const sfizz_arg_t* args)
{
    if (!strcmp(path, "/sw/last/current") && sig)
    {
        if (sig[0] == 'i')
            _state.lastKeyswitch = args[0].i;
        else if (sig[0] == 'N')
            _state.lastKeyswitch = -1;
    }

    uint8_t* oscTemp = _oscTemp.get();
    uint32 oscSize = sfizz_prepare_message(oscTemp, kOscTempSize, path, sig, args);
    if (oscSize <= kOscTempSize) {
        if (writeWorkerMessage(kMsgIdReceiveOSC, oscTemp, oscSize))
            _semaToWorker.post();
    }
}

void SfizzVstProcessor::loadSfzFileOrDefault(const std::string& filePath, bool initParametersFromState)
{
    sfz::Sfizz& synth = *_synth;

    if (!filePath.empty()) {
        sfizz_load_or_import_file(synth.handle(), filePath.c_str(), nullptr);
    }
    else {
        synth.loadSfzString("default.sfz", defaultSfzText);
    }

    const std::string descBlob = getDescriptionBlob(synth.handle());
    {
        std::vector<absl::optional<float>> newControllers(sfz::config::numCCs);
        const std::vector<absl::optional<float>> oldControllers = std::move(_state.controllers);
        // collect initial CC from instrument
        const InstrumentDescription desc = parseDescriptionBlob(descBlob);
        for (uint32 cc = 0; cc < sfz::config::numCCs; ++cc) {
            if (desc.ccUsed.test(cc))
                newControllers[cc] = desc.ccValue[cc];
        }
        // set CC from existing state
        if (initParametersFromState) {
            for (uint32 cc = 0; cc < sfz::config::numCCs; ++cc) {
                if (absl::optional<float> value = oldControllers[cc]) {
                    newControllers[cc] = *value;
                    synth.automateHdcc(0, int(cc), *value);
                }
            }
        }
        _state.controllers = std::move(newControllers);
    }

    // create a message which requests the controller to automate initial parameters
    std::vector<AutomationUpdate::Item> automationItems;
    automationItems.reserve(sfz::config::numCCs);
    for (uint32 cc = 0; cc < sfz::config::numCCs; ++cc) {
        Vst::ParamID pid = kPidCC0 + cc;
        float value = _state.controllers[cc].value_or(0.0f);
        automationItems.emplace_back(pid, value);
    }

    // send message
    _sfzUpdate->setPath(filePath);
    _sfzUpdate->deferUpdate();
    _sfzDescriptionUpdate->setDescription(descBlob);
    _sfzDescriptionUpdate->deferUpdate();
    _automationUpdate->setItems(std::move(automationItems));
    _automationUpdate->deferUpdate();
}

void SfizzVstProcessor::doBackgroundWork()
{
    using Clock = std::chrono::steady_clock;

    bool haveDoneIdleWork = false;
    Clock::time_point lastIdleWorkTime;
    size_t idleCounter = 0;

    for (;;) {
        bool isNotified = _semaToWorker.timed_wait(kBackgroundIdleInterval.count());

        if (!_workRunning) {
            // if the quit signal is sent, the semaphore is also signaled
            // make sure the count is kept consistent
            if (!isNotified)
                _semaToWorker.wait();
            break;
        }

        const char* id = nullptr;
        RTMessagePtr msg;

        if (isNotified) {
            msg = readWorkerMessage();
            if (!msg) {
                fprintf(stderr, "[Sfizz] message synchronization error in worker\n");
                std::abort();
            }
            id = msg->type;
        }

        if (id == kMsgIdSetNumVoices) {
            int32 value = *msg->payload<int32>();
            std::lock_guard<SpinMutex> lock(_processMutex);
            _synth->setNumVoices(value);
        }
        else if (id == kMsgIdSetOversampling) {
            int32 value = *msg->payload<int32>();
            std::lock_guard<SpinMutex> lock(_processMutex);
            _synth->setOversamplingFactor(1 << value);
        }
        else if (id == kMsgIdSetPreloadSize) {
            int32 value = *msg->payload<int32>();
            std::lock_guard<SpinMutex> lock(_processMutex);
            _synth->setPreloadSize(value);
        }
        else if (id == kMsgIdReceiveOSC) {
            IPtr<OSCUpdate> update = Steinberg::owned(
                new OSCUpdate(msg->payload<uint8>(), msg->size));
            _queuedMessages->enqueue(update);
            _queuedMessages->deferUpdate();
        }
        else if (id == kMsgIdNoteEvents) {
            IPtr<NoteUpdate> update = Steinberg::owned(
                new NoteUpdate(msg->payload<NoteUpdate::Item>(), msg->size / sizeof(NoteUpdate::Item)));
            _queuedMessages->enqueue(update);
            _queuedMessages->deferUpdate();
        }

        Clock::time_point currentTime = Clock::now();
        if (!haveDoneIdleWork || currentTime - lastIdleWorkTime > kBackgroundIdleInterval) {
            doBackgroundIdle(idleCounter++);
            haveDoneIdleWork = true;
            lastIdleWorkTime = currentTime;
        }
    }
}

void SfizzVstProcessor::doBackgroundIdle(size_t idleCounter)
{
    {
        SfizzPlayState ps;
        ps.activeVoices = _synth->getNumActiveVoices();
        _playStateUpdate->setState(ps);
        _playStateUpdate->deferUpdate();
    }

    if (idleCounter % 25 == 0) {
        if (_synth->shouldReloadFile()) {
            fprintf(stderr, "[Sfizz] sfz file has changed, reloading\n");
            std::lock_guard<SpinMutex> lock(_processMutex);
            loadSfzFileOrDefault(_state.sfzFile, false);
        }
        if (_synth->shouldReloadScala()) {
            fprintf(stderr, "[Sfizz] scala file has changed, reloading\n");
            std::lock_guard<SpinMutex> lock(_processMutex);
            _synth->loadScalaFile(_state.scalaFile);
        }
    }
}

void SfizzVstProcessor::startBackgroundWork()
{
    if (_workRunning)
        return;

    _workRunning = true;
    _worker = std::thread([this]() { doBackgroundWork(); });
}

void SfizzVstProcessor::stopBackgroundWork()
{
    if (!_workRunning)
        return;

    _workRunning = false;
    _semaToWorker.post();
    _worker.join();

    while (_semaToWorker.try_wait()) {
        if (!discardWorkerMessage()) {
            fprintf(stderr, "[Sfizz] message synchronization error in processor\n");
            std::abort();
        }
    }
}

bool SfizzVstProcessor::writeWorkerMessage(const char* type, const void* data, uintptr_t size)
{
    return writeMessage(_fifoToWorker, type, data, size);
}

SfizzVstProcessor::RTMessagePtr SfizzVstProcessor::readWorkerMessage()
{
    RTMessage header;

    if (!_fifoToWorker.peek(header))
        return nullptr;
    if (_fifoToWorker.size_used() < sizeof(header) + header.size)
        return nullptr;

    RTMessagePtr msg { reinterpret_cast<RTMessage*>(std::malloc(sizeof(header) + header.size)) };
    if (!msg)
        throw std::bad_alloc();

    msg->type = header.type;
    msg->size = header.size;
    _fifoToWorker.discard(sizeof(header));
    _fifoToWorker.get(const_cast<char*>(msg->payload<char>()), header.size);

    return msg;
}

bool SfizzVstProcessor::discardWorkerMessage()
{
    RTMessage header;

    if (!_fifoToWorker.peek(header))
        return false;
    if (_fifoToWorker.size_used() < sizeof(header) + header.size)
        return false;

    _fifoToWorker.discard(sizeof(header) + header.size);
    return true;
}

bool SfizzVstProcessor::writeMessage(Ring_Buffer& fifo, const char* type, const void* data, uintptr_t size)
{
    RTMessage header;
    header.type = type;
    header.size = size;

    if (fifo.size_free() < sizeof(header) + size)
        return false;

    fifo.put(header);
    fifo.put(static_cast<const uint8*>(data), size);
    return true;
}

FUnknown* SfizzVstProcessor::createInstance(void*)
{
    return static_cast<Vst::IAudioProcessor*>(new SfizzVstProcessor);
}

FUnknown* SfizzVstProcessorMulti::createInstance(void*)
{
    return static_cast<Vst::IAudioProcessor*>(new SfizzVstProcessorMulti);
}

template <>
FUnknown* createInstance<SfizzVstProcessor>(void* context)
{
    return SfizzVstProcessor::createInstance(context);
}

tresult PLUGIN_API SfizzVstProcessorMulti::initialize(FUnknown* context)
{
    tresult res = SfizzVstProcessor::initialize(context);
    if (res != kResultFalse) {
        addAudioOutput(STR16("Audio Output 2"), Vst::SpeakerArr::kStereo);
        addAudioOutput(STR16("Audio Output 3"), Vst::SpeakerArr::kStereo);
        addAudioOutput(STR16("Audio Output 4"), Vst::SpeakerArr::kStereo);
        addAudioOutput(STR16("Audio Output 5"), Vst::SpeakerArr::kStereo);
        addAudioOutput(STR16("Audio Output 6"), Vst::SpeakerArr::kStereo);
        addAudioOutput(STR16("Audio Output 7"), Vst::SpeakerArr::kStereo);
        addAudioOutput(STR16("Audio Output 8"), Vst::SpeakerArr::kStereo);
    }
    _multi = true;
    _rmsFollower.setNumOutputs(16);
    return res;
}

template <>
FUnknown* createInstance<SfizzVstProcessorMulti>(void* context)
{
    return SfizzVstProcessorMulti::createInstance(context);
}

FUID SfizzVstProcessor::cid = SfizzVstProcessor_cid;
FUID SfizzVstProcessorMulti::cid = SfizzVstProcessor_cid;
