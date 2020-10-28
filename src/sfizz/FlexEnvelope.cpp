// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/*
   Note(jpc): implementation status

- [ ] egN_points (purpose unknown)
- [x] egN_timeX
- [x] egN_levelX
- [x] egN_shapeX
- [x] egN_sustain
- [ ] egN_dynamic
- [ ] egN_loop
- [ ] egN_loop_shape
- [ ] egN_loop_count
*/

#include "FlexEnvelope.h"
#include "FlexEGDescription.h"
#include "Curve.h"
#include "Config.h"
#include "SIMDHelpers.h"
#include <absl/types/optional.h>

namespace sfz {

struct FlexEnvelope::Impl {
    const FlexEGDescription* desc_ { nullptr };
    float samplePeriod_ { 1.0 / config::defaultSampleRate };
    size_t delayFramesLeft_ { 0 };

    //
    float stageSourceLevel_ { 0.0 };
    float stageTargetLevel_ { 0.0 };
    float stageTime_ { 0.0 };
    bool stageSustained_ { false };
    const Curve* stageCurve_ { nullptr };

    //
    unsigned currentStageNumber_ { 0 };
    float currentLevel_ { 0.0 };
    float currentTime_ { 0.0 };
    absl::optional<size_t> currentFramesUntilRelease_ { absl::nullopt };
    bool isReleased_ { false };
    bool freeRunning_ { false };

    //
    void process(absl::Span<float> out);
    bool advanceToNextStage();
};

FlexEnvelope::FlexEnvelope()
    : impl_(new Impl)
{
}

FlexEnvelope::~FlexEnvelope()
{
}

void FlexEnvelope::setSampleRate(double sampleRate)
{
    Impl& impl = *impl_;
    impl.samplePeriod_ = 1.0 / sampleRate;
}

void FlexEnvelope::configure(const FlexEGDescription* desc)
{
    Impl& impl = *impl_;
    impl.desc_ = desc;

    //
    impl.freeRunning_ = false;
    impl.isReleased_ = false;

    //
    impl.currentStageNumber_ = 0;
    impl.currentLevel_ = 0.0;
    impl.currentTime_ = 0.0;
}

void FlexEnvelope::start(unsigned triggerDelay)
{
    Impl& impl = *impl_;
    const FlexEGDescription& desc = *impl.desc_;

    impl.delayFramesLeft_ = triggerDelay;

    FlexEGPoint point;
    if (!desc.points.empty())
        point = desc.points[0];

    //
    impl.stageSourceLevel_ = 0.0;
    impl.stageTargetLevel_ = point.level;
    impl.stageTime_ = point.time;
    impl.stageSustained_ = desc.sustain == 0;
    impl.stageCurve_ = &point.curve();
    impl.currentFramesUntilRelease_ = absl::nullopt;
}

void FlexEnvelope::setFreeRunning(bool freeRunning)
{
    Impl& impl = *impl_;
    impl.freeRunning_ = freeRunning;
}

void FlexEnvelope::release(unsigned releaseDelay)
{
    Impl& impl = *impl_;
    impl.currentFramesUntilRelease_ = releaseDelay;
}

unsigned FlexEnvelope::getRemainingDelay() const noexcept
{
    const Impl& impl = *impl_;
    return static_cast<unsigned>(impl.delayFramesLeft_);
}

bool FlexEnvelope::isReleased() const noexcept
{
    const Impl& impl = *impl_;
    return impl.isReleased_;
}

bool FlexEnvelope::isFinished() const noexcept
{
    const Impl& impl = *impl_;
    const FlexEGDescription& desc = *impl.desc_;
    return impl.currentStageNumber_ >= desc.points.size();
}

void FlexEnvelope::process(absl::Span<float> out)
{
    Impl& impl = *impl_;
    impl.process(out);
}

void FlexEnvelope::Impl::process(absl::Span<float> out)
{
    const FlexEGDescription& desc = *desc_;
    size_t numFrames = out.size();
    const float samplePeriod = samplePeriod_;
    // Skip the initial delay, for frame-accurate trigger
    size_t skipFrames = std::min(numFrames, delayFramesLeft_);
    if (skipFrames > 0) {
        delayFramesLeft_ -= skipFrames;
        fill(absl::MakeSpan(out.data(), skipFrames), 0.0f);
        out.remove_prefix(skipFrames);
        numFrames -= skipFrames;
    }

    // Envelope finished?
    if (currentStageNumber_ >= desc.points.size()) {
        fill(out, 0.0f);
        return;
    }

    size_t frameIndex = 0;

    while (frameIndex < numFrames) {
        // Check for release
        if (currentFramesUntilRelease_ && *currentFramesUntilRelease_ == 0) {
            isReleased_ = true;
            currentFramesUntilRelease_ = absl::nullopt;
        }

        // Perform stage transitions
        if (isReleased_) {
            // on release, fast forward past the sustain stage
            const unsigned sustainStage = desc.sustain;
            while (currentStageNumber_ <= sustainStage) {
                if (!advanceToNextStage()) {
                    out.remove_prefix(frameIndex);
                    fill(out, 0.0f);
                    return;
                }
            }
        }
        while ((!stageSustained_ || freeRunning_) && currentTime_ >= stageTime_) {
            // advance through completed timed stages
            ASSERT(isReleased_ || !stageSustained_ || freeRunning_);
            if (stageTime_ == 0) {
                // if stage is of zero duration, immediate transition to level
                currentLevel_ = stageTargetLevel_;
            }
            if (!advanceToNextStage()) {
                out.remove_prefix(frameIndex);
                fill(out, 0.0f);
                return;
            }
        }

        // Process without going past the release point, if there is one
        size_t maxFrameIndex = numFrames;
        if (currentFramesUntilRelease_)
            maxFrameIndex = std::min(maxFrameIndex, frameIndex + *currentFramesUntilRelease_);

        // Process the current stage
        float time = currentTime_;
        float level = currentLevel_;
        const float stageEndTime = stageTime_;
        const float sourceLevel = stageSourceLevel_;
        const float targetLevel = stageTargetLevel_;
        const bool sustained = stageSustained_;
        const Curve& curve = *stageCurve_;
        size_t framesDone = 0;
        while ((time < stageEndTime || sustained) && frameIndex < maxFrameIndex) {
            time += samplePeriod;
            float x = time * (1.0f / stageEndTime);
            float c = curve.evalNormalized(x);
            level = sourceLevel + c * (targetLevel - sourceLevel);
            out[frameIndex++] = level;
            ++framesDone;
        }
        currentLevel_ = level;

        // Update the counter to release
        if (currentFramesUntilRelease_)
            *currentFramesUntilRelease_ -= framesDone;

        currentTime_ = time;
    }
}

bool FlexEnvelope::Impl::advanceToNextStage()
{
    const FlexEGDescription& desc = *desc_;

    unsigned nextStageNo = currentStageNumber_ + 1;
    currentStageNumber_ = nextStageNo;

    if (nextStageNo >= desc.points.size())
        return false;

    const FlexEGPoint& point = desc.points[nextStageNo];
    stageSourceLevel_ = currentLevel_;
    stageTargetLevel_ = point.level;
    stageTime_ = point.time;
    stageSustained_ = int(nextStageNo) == desc.sustain;
    stageCurve_ = &point.curve();

    currentTime_ = 0;
    return true;
};

} // namespace sfz
