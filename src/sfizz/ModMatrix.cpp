// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ModMatrix.h"
#include "Config.h"
#include "StringViewHelpers.h"
#include "SIMDHelpers.h"
#include "Debug.h"
#include <algorithm>
#include <cassert>

size_t std::hash<sfz::ModKey>::operator()(const sfz::ModKey &key) const
{
    uint64_t k = key.id;
    for (float x : key.params)
        hashNumber(x, k);
    return k;
}

bool sfz::ModKey::operator==(const ModKey &other) const noexcept
{
    return id == other.id && params == other.params;
}

bool sfz::ModKey::operator!=(const ModKey &other) const noexcept
{
    return !this->operator==(other);
}

namespace sfz {

ModMatrix::ModMatrix()
    : _samplesPerBlock(config::defaultSamplesPerBlock)
{
    _temp.resize(_samplesPerBlock);
}

ModMatrix::~ModMatrix()
{
}

void ModMatrix::clear()
{
    _sourceIndex.clear();
    _targetIndex.clear();
    _sources.clear();
    _targets.clear();
}

void ModMatrix::setSamplesPerBlock(unsigned samplesPerBlock)
{
    _samplesPerBlock = samplesPerBlock;

    for (Source &source : _sources)
        source.buffer.resize(samplesPerBlock);
    for (Target &target : _targets)
        target.buffer.resize(samplesPerBlock);

    _temp.resize(samplesPerBlock);
}

ModMatrix::SourceId ModMatrix::registerSource(ModKey key, ModGenerator* gen, bool genOwned, int32_t flags)
{
    assert(gen);

    std::unique_ptr<ModGenerator> genOwnerPtr;
    if (genOwned)
        genOwnerPtr.reset(gen);

    auto it = _sourceIndex.find(key);
    if (it != _sourceIndex.end()) {
        SourceId id;
        id.index = it->second;
        if (_sources[id.index].flags != flags)
            DBG("Source flags do not match the existing entry");
        return id;
    }

    SourceId id;
    id.index = _sources.size();
    _sources.emplace_back();

    Source &source = _sources.back();
    source.key = key;
    source.gen = gen;
    source.genOwnerPtr = std::move(genOwnerPtr);
    source.flags = flags;
    source.bufferReady = false;
    source.buffer.resize(_samplesPerBlock);

    _sourceIndex[key] = id.index;
    return id;
}

ModMatrix::TargetId ModMatrix::registerTarget(ModKey key, uint32_t region, int32_t flags)
{
    auto it = _targetIndex.find(std::make_pair(key, region));
    if (it != _targetIndex.end()) {
        TargetId id;
        id.index = it->second;
        if (_targets[id.index].flags != flags)
            DBG("Target flags do not match the existing entry");
        return id;
    }

    TargetId id;
    id.index = _targets.size();
    _targets.emplace_back();

    Target &target = _targets.back();
    target.key = key;
    target.flags = flags;
    target.bufferReady = false;
    target.buffer.resize(_samplesPerBlock);

    _targetIndex[std::make_pair(key, region)] = id.index;
    return id;
}

ModMatrix::SourceId ModMatrix::findSource(ModKey key)
{
    SourceId id;

    auto it = _sourceIndex.find(key);
    if (it != _sourceIndex.end())
        id.index = it->second;
    else
        id.index = -1;

    return id;
}

ModMatrix::TargetId ModMatrix::findTarget(ModKey key, uint32_t region)
{
    TargetId id;

    auto it = _targetIndex.find(std::make_pair(key, region));
    if (it != _targetIndex.end())
        id.index = it->second;
    else
        id.index = -1;

    return id;
}

bool ModMatrix::connect(SourceId sourceId, TargetId targetId, float depth)
{
    uint32_t sourceIndex = sourceId.index;
    uint32_t targetIndex = targetId.index;

    if (sourceIndex >= _sources.size() || targetIndex >= _targets.size())
        return false;

    Target& target = _targets[targetIndex];
    ConnectionData& conn = target.connectedSources[sourceIndex];
    conn.depthToTarget = depth;

    return true;
}

void ModMatrix::beginCycle(unsigned numFrames)
{
    _numFrames = numFrames;

    for (Source &source : _sources)
        source.bufferReady = false;
    for (Target &target : _targets)
        target.bufferReady = false;
}

void ModMatrix::beginVoice(unsigned voiceNum)
{
    _voiceNum = voiceNum;

    for (Source &source : _sources) {
        if (source.flags & kModIsPerVoice)
            source.bufferReady = false;
    }
    for (Target &target : _targets) {
        if (target.flags & kModIsPerVoice)
            target.bufferReady = false;
    }
}

float* ModMatrix::getModulation(TargetId targetId)
{
    if (!validTarget(targetId))
        return nullptr;

    const uint32_t targetIndex = targetId.index;
    Target &target = _targets[targetIndex];

    absl::Span<float> buffer = absl::MakeSpan(target.buffer);

    // check if already processed
    if (target.bufferReady)
        return buffer.data();

    const uint32_t numFrames = _numFrames;

    // set the ready flag to prevent a cycle
    // in case there is, be sure to initialize the buffer
    target.bufferReady = true;
    const float neutralElement = (target.flags & kModIsMultiplicative) ? 1.0f : 0.0f;
    sfz::fill(buffer, neutralElement);

    // const std::vector<uint32_t> &sourceIndices = target.sources;
    auto sourcesPos = target.connectedSources.begin();
    auto sourcesEnd = target.connectedSources.end();

    // generate the first source in buffer
    if (sourcesPos != sourcesEnd) {
        Source &source = _sources[sourcesPos->first];
        const float depth = sourcesPos->second.depthToTarget;
        source.gen->generateModulation(source.key, _voiceNum, buffer.data(), numFrames);
        for (uint32_t i = 0; i < numFrames; ++i)
            buffer[i] *= depth;
        ++sourcesPos;
    }

    // generate next sources in temporary buffer
    // then add or multiply, depending on target flags
    absl::Span<float> temp = absl::MakeSpan(_temp);
    while (sourcesPos != sourcesEnd) {
        Source &source = _sources[sourcesPos->first];
        const float depth = sourcesPos->second.depthToTarget;
        source.gen->generateModulation(source.key, _voiceNum, temp.data(), numFrames);
        if (target.flags & kModIsMultiplicative) {
            for (uint32_t i = 0; i < numFrames; ++i)
                buffer[i] *= depth * temp[i];
        }
        else {
            for (uint32_t i = 0; i < numFrames; ++i)
                buffer[i] += depth * temp[i];
        }
        ++sourcesPos;
    }

    return buffer.data();
}

} // namespace sfz
