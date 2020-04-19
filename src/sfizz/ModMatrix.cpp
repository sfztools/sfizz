// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ModMatrix.h"
#include "Config.h"
#include "StringViewHelpers.h"
#include "SIMDHelpers.h"
#include <algorithm>
#include <cassert>

size_t std::hash<sfz::ModKey>::operator()(const sfz::ModKey &key) const
{
    auto hashableInt32 = [](const int32_t *p) -> absl::string_view {
        return absl::string_view(reinterpret_cast<const char *>(p), sizeof(*p));
    };
    uint64_t k = key.id;
    k = ::hash(hashableInt32(&key.index1), k);
    k = ::hash(hashableInt32(&key.index2), k);
    return k;
}

bool sfz::ModKey::operator==(const ModKey &other) const noexcept
{
    return id == other.id && index1 == other.index1 && index2 == other.index2;
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

ModMatrix::TargetId ModMatrix::registerTarget(ModKey key, int32_t flags)
{
    auto it = _targetIndex.find(key);
    if (it != _targetIndex.end()) {
        TargetId id;
        id.index = it->second;
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

    _targetIndex[key] = id.index;
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

ModMatrix::TargetId ModMatrix::findTarget(ModKey key)
{
    TargetId id;

    auto it = _targetIndex.find(key);
    if (it != _targetIndex.end())
        id.index = it->second;
    else
        id.index = -1;

    return id;
}

bool ModMatrix::connect(SourceId sourceId, TargetId targetId)
{
    uint32_t sourceIndex = sourceId.index;
    uint32_t targetIndex = targetId.index;

    if (sourceIndex >= _sources.size() || targetIndex >= _targets.size())
        return false;

    Target& target = _targets[targetIndex];
    std::vector<uint32_t> &list = target.sources;

    // add the source if not already present
    if (std::find(list.begin(), list.end(), sourceIndex) == list.end())
        list.push_back(sourceIndex);

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
    sfz::fill(buffer, 0.0f);

    const std::vector<uint32_t> &sourceIndices = target.sources;
    const size_t numSources = sourceIndices.size();

    // generate the first source in buffer
    if (numSources > 0) {
        Source &source = _sources[sourceIndices[0]];
        source.gen->generateModulation(source.key, _voiceNum, buffer.data(), numFrames);
    }

    // generate next sources in temporary buffer
    // then add or multiply, depending on target flags
    absl::Span<float> temp = absl::MakeSpan(_temp);
    for (uint32_t s = 1; s < numSources; ++s) {
        Source &source = _sources[sourceIndices[s]];
        source.gen->generateModulation(source.key, _voiceNum, temp.data(), numFrames);
        if (target.flags & kModIsMultiplicative) {
            for (uint32_t i = 0; i < numFrames; ++i)
                buffer[i] *= temp[i];
        }
        else {
            for (uint32_t i = 0; i < numFrames; ++i)
                buffer[i] += temp[i];
        }
    }

    return buffer.data();
}

} // namespace sfz
