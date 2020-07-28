// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ModMatrix.h"
#include "ModId.h"
#include "ModKey.h"
#include "ModGenerator.h"
#include "Buffer.h"
#include "Config.h"
#include "SIMDHelpers.h"
#include "Debug.h"
#include <absl/container/flat_hash_map.h>
#include <vector>
#include <algorithm>

namespace sfz {

struct ModMatrix::Impl {
    double sampleRate_ {};
    uint32_t samplesPerBlock_ {};

    uint32_t numFrames_ {};
    NumericId<Voice> voiceId_ {};
    NumericId<Region> regionId_ {};

    struct Source {
        ModKey key;
        ModGenerator* gen {};
        bool bufferReady {};
        Buffer<float> buffer;
    };

    struct ConnectionData {
        float sourceDepth_ {};
    };

    struct Target {
        ModKey key;
        uint32_t region {};
        absl::flat_hash_map<uint32_t, ConnectionData> connectedSources;
        bool bufferReady {};
        Buffer<float> buffer;
    };

    absl::flat_hash_map<ModKey, uint32_t> sourceIndex_;
    absl::flat_hash_map<ModKey, uint32_t> targetIndex_;

    std::vector<Source> sources_;
    std::vector<Target> targets_;

    Buffer<float> temp_;
};

ModMatrix::ModMatrix()
    : impl_(new Impl)
{
    setSampleRate(config::defaultSampleRate);
    setSamplesPerBlock(config::defaultSamplesPerBlock);
}

ModMatrix::~ModMatrix()
{
}

void ModMatrix::clear()
{
    Impl& impl = *impl_;

    impl.sourceIndex_.clear();
    impl.targetIndex_.clear();
    impl.sources_.clear();
    impl.targets_.clear();
}

void ModMatrix::setSampleRate(double sampleRate)
{
    Impl& impl = *impl_;

    if (impl.sampleRate_ == sampleRate)
        return;

    impl.sampleRate_ = sampleRate;

    for (Impl::Source &source : impl.sources_)
        source.gen->setSampleRate(sampleRate);
}

void ModMatrix::setSamplesPerBlock(unsigned samplesPerBlock)
{
    Impl& impl = *impl_;

    if (impl.samplesPerBlock_ == samplesPerBlock)
        return;

    impl.samplesPerBlock_ = samplesPerBlock;

    for (Impl::Source &source : impl.sources_) {
        source.buffer.resize(samplesPerBlock);
        source.gen->setSamplesPerBlock(samplesPerBlock);
    }
    for (Impl::Target &target : impl.targets_)
        target.buffer.resize(samplesPerBlock);

    impl.temp_.resize(samplesPerBlock);
}

ModMatrix::SourceId ModMatrix::registerSource(const ModKey& key, ModGenerator& gen)
{
    Impl& impl = *impl_;

    auto it = impl.sourceIndex_.find(key);
    if (it != impl.sourceIndex_.end()) {
        ASSERT(&gen == impl.sources_[it->second].gen);
        return SourceId(it->second);
    }

    SourceId id(static_cast<int>(impl.sources_.size()));
    impl.sources_.emplace_back();

    Impl::Source &source = impl.sources_.back();
    source.key = key;
    source.gen = &gen;
    source.bufferReady = false;
    source.buffer.resize(impl.samplesPerBlock_);

    impl.sourceIndex_[key] = id.number();

    gen.setSampleRate(impl.sampleRate_);
    gen.setSamplesPerBlock(impl.samplesPerBlock_);

    return id;
}

ModMatrix::TargetId ModMatrix::registerTarget(const ModKey& key)
{
    Impl& impl = *impl_;

    auto it = impl.targetIndex_.find(key);
    if (it != impl.targetIndex_.end())
        return TargetId(it->second);

    TargetId id(static_cast<int>(impl.targets_.size()));
    impl.targets_.emplace_back();

    Impl::Target &target = impl.targets_.back();
    target.key = key;
    target.bufferReady = false;
    target.buffer.resize(impl.samplesPerBlock_);

    impl.targetIndex_[key] = id.number();
    return id;
}

ModMatrix::SourceId ModMatrix::findSource(const ModKey& key)
{
    Impl& impl = *impl_;

    auto it = impl.sourceIndex_.find(key);
    if (it == impl.sourceIndex_.end())
        return {};

    return SourceId(it->second);
}

ModMatrix::TargetId ModMatrix::findTarget(const ModKey& key)
{
    Impl& impl = *impl_;

    auto it = impl.targetIndex_.find(key);
    if (it == impl.targetIndex_.end())
        return {};

    return TargetId(it->second);
}

bool ModMatrix::connect(SourceId sourceId, TargetId targetId, float sourceDepth)
{
    Impl& impl = *impl_;
    unsigned sourceIndex = sourceId.number();
    unsigned targetIndex = targetId.number();

    if (sourceIndex >= impl.sources_.size() || targetIndex >= impl.targets_.size())
        return false;

    Impl::Target& target = impl.targets_[targetIndex];
    Impl::ConnectionData& conn = target.connectedSources[sourceIndex];
    conn.sourceDepth_ = sourceDepth;

    return true;
}

void ModMatrix::init()
{
    Impl& impl = *impl_;

    for (Impl::Source &source : impl.sources_) {
        const int flags = source.key.flags();
        if (flags & kModIsPerCycle)
            source.gen->init(source.key, {});
    }
}

void ModMatrix::initVoice(NumericId<Voice> voiceId, NumericId<Region> regionId)
{
    Impl& impl = *impl_;

    for (Impl::Source &source : impl.sources_) {
        const int flags = source.key.flags();
        if ((flags & kModIsPerVoice) && source.key.region() == regionId)
            source.gen->init(source.key, voiceId);
    }
}

void ModMatrix::beginCycle(unsigned numFrames)
{
    Impl& impl = *impl_;

    impl.numFrames_ = numFrames;

    for (Impl::Source &source : impl.sources_)
        source.bufferReady = false;
    for (Impl::Target &target : impl.targets_)
        target.bufferReady = false;
}

void ModMatrix::endCycle()
{
    Impl& impl = *impl_;
    const uint32_t numFrames = impl.numFrames_;

    for (Impl::Source &source : impl.sources_) {
        if (!source.bufferReady) {
            const int flags = source.key.flags();
            if (flags & kModIsPerCycle) {
                absl::Span<float> buffer(source.buffer.data(), numFrames);
                source.gen->generateDiscarded(source.key, {}, buffer);
            }
        }
    }

    impl.numFrames_ = 0;
}

void ModMatrix::beginVoice(NumericId<Voice> voiceId, NumericId<Region> regionId)
{
    Impl& impl = *impl_;

    impl.voiceId_ = voiceId;
    impl.regionId_ = regionId;

    for (Impl::Source &source : impl.sources_) {
        const int flags = source.key.flags();
        if (flags & kModIsPerVoice)
            source.bufferReady = false;
    }
    for (Impl::Target &target : impl.targets_) {
        const int flags = target.key.flags();
        if (flags & kModIsPerVoice)
            target.bufferReady = false;
    }
}

void ModMatrix::endVoice()
{
    Impl& impl = *impl_;
    const uint32_t numFrames = impl.numFrames_;
    const NumericId<Voice> voiceId = impl.voiceId_;
    const NumericId<Region> regionId = impl.regionId_;

    for (Impl::Source &source : impl.sources_) {
        if (!source.bufferReady) {
            const int flags = source.key.flags();
            if ((flags & kModIsPerVoice) && source.key.region() == regionId) {
                absl::Span<float> buffer(source.buffer.data(), numFrames);
                source.gen->generateDiscarded(source.key, voiceId, buffer);
            }
        }
    }

    impl.voiceId_ = {};
    impl.regionId_ = {};
}

float* ModMatrix::getModulation(TargetId targetId)
{
    if (!validTarget(targetId))
        return nullptr;

    Impl& impl = *impl_;
    const NumericId<Region> regionId = impl.regionId_;
    const uint32_t targetIndex = targetId.number();
    Impl::Target &target = impl.targets_[targetIndex];
    const int targetFlags = target.key.flags();

    const uint32_t numFrames = impl.numFrames_;
    absl::Span<float> buffer(target.buffer.data(), numFrames);

    // only accept per-voice targets of the same region
    if ((targetFlags & kModIsPerVoice) && regionId != target.key.region())
        return nullptr;

    // check if already processed
    if (target.bufferReady)
        return buffer.data();

    // set the ready flag to prevent a cycle
    // in case there is, be sure to initialize the buffer
    target.bufferReady = true;

    auto sourcesPos = target.connectedSources.begin();
    auto sourcesEnd = target.connectedSources.end();
    bool isFirstSource = true;

    // generate first source in output buffer, next sources in temporary buffer
    // then add or multiply, depending on target flags
    while (sourcesPos != sourcesEnd) {
        Impl::Source &source = impl.sources_[sourcesPos->first];
        const float sourceDepth = sourcesPos->second.sourceDepth_;
        const int sourceFlags = source.key.flags();

        // only accept per-voice sources of the same region
        bool useThisSource = true;
        if (sourceFlags & kModIsPerVoice)
            useThisSource = (regionId == source.key.region());

        if (useThisSource) {
            if (isFirstSource) {
                source.gen->generate(source.key, impl.voiceId_, buffer);
                isFirstSource = false;
            }
            else {
                absl::Span<float> temp(impl.temp_.data(), numFrames);
                source.gen->generate(source.key, impl.voiceId_, temp);
                if (targetFlags & kModIsMultiplicative) {
                    for (uint32_t i = 0; i < numFrames; ++i)
                        buffer[i] *= sourceDepth * temp[i];
                }
                else if (targetFlags & kModIsPercentMultiplicative) {
                    for (uint32_t i = 0; i < numFrames; ++i)
                        buffer[i] *= (0.01f * sourceDepth) * temp[i];
                }
                else {
                    ASSERT(targetFlags & kModIsAdditive);
                    for (uint32_t i = 0; i < numFrames; ++i)
                        buffer[i] += sourceDepth * temp[i];
                }
            }
        }

        ++sourcesPos;
    }

    // if there were no source, fill output with the neutral element
    if (isFirstSource) {
        if (targetFlags & kModIsMultiplicative)
            sfz::fill(buffer, 1.0f);
        else if (targetFlags & kModIsPercentMultiplicative)
            sfz::fill(buffer, 100.0f);
        else {
            ASSERT(targetFlags & kModIsAdditive);
            sfz::fill(buffer, 0.0f);
        }
    }

    return buffer.data();
}

bool ModMatrix::validTarget(TargetId id) const
{
    return static_cast<unsigned>(id.number()) < impl_->targets_.size();
}

bool ModMatrix::validSource(SourceId id) const
{
    return static_cast<unsigned>(id.number()) < impl_->sources_.size();
}

} // namespace sfz
