// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Buffer.h"
#include "Config.h"
#include "Debug.h"
#include "LeakDetector.h"
#include "absl/types/span.h"
#include "absl/memory/memory.h"
#include <array>

namespace sfz
{
/**
 * @brief A class to handle a collection of buffers, where each buffer has the same size.
 *
 * Unlike AudioSpan, this class *owns* its underlying buffers and they are freed when the buffer
 * is destroyed.
 *
 * @tparam Type the underlying type of the buffers
 * @tparam MaxChannels the maximum number of channels in the buffer
 * @tparam Alignment the alignment for the buffers
 */
template <class Type, size_t MaxChannels = sfz::config::numChannels, unsigned int Alignment = SIMDConfig::defaultAlignment>
class AudioBuffer {
public:
    using value_type = typename std::remove_cv<Type>::type;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    /**
     * @brief Construct a new Audio Buffer object
     *
     */
    AudioBuffer()
    {
    }

    /**
     * @brief Construct a new Audio Buffer object with a specified number of
     * channels and frames.
     *
     * @param numChannels
     * @param numFrames
     */
    AudioBuffer(size_t numChannels, size_t numFrames)
        : numChannels(numChannels)
        , numFrames(numFrames)
    {
        for (size_t i = 0; i < numChannels; ++i)
            buffers[i] = absl::make_unique<buffer_type>(numFrames);
    }

    /**
     * @brief Resizes all the underlying buffers to a new size.
     *
     * @param newSize
     * @return true if the resize worked
     * @return false otherwise
     */
    bool resize(size_t newSize)
    {
        bool returnedOK = true;

        for (size_t i = 0; i < numChannels; ++i)
            returnedOK &= buffers[i]->resize(newSize);

        if (returnedOK)
            numFrames = newSize;

        return returnedOK;
    }

    /**
     * @brief Return an iterator to a specific channel with a non-const type.
     *
     * @param channelIndex
     * @return iterator
     */
    iterator channelWriter(size_t channelIndex)
    {
        ASSERT(channelIndex < numChannels)
        if (channelIndex < numChannels)
            return buffers[channelIndex]->data();

        return {};
    }

    /**
     * @brief Returns a sentinel for the channelWriter(channelIndex) iterator
     *
     * @param channelIndex
     * @return iterator
     */
    iterator channelWriterEnd(size_t channelIndex)
    {
        ASSERT(channelIndex < numChannels)
        if (channelIndex < numChannels)
            return buffers[channelIndex]->end();

        return {};
    }

    /**
     * @brief Returns a const iterator for a specific channel
     *
     * @param channelIndex
     * @return const_iterator
     */
    const_iterator channelReader(size_t channelIndex) const
    {
        ASSERT(channelIndex < numChannels)
        if (channelIndex < numChannels)
            return buffers[channelIndex]->data();

        return {};
    }

    /**
     * @brief Returns a sentinel for the channelReader(channelIndex) iterator
     *
     * @param channelIndex
     * @return const_iterator
     */
    const_iterator channelReaderEnd(size_t channelIndex) const
    {
        ASSERT(channelIndex < numChannels)
        if (channelIndex < numChannels)
            return buffers[channelIndex]->end();

        return {};
    }

    /**
     * @brief Get a Span for a specific channel
     *
     * @param channelIndex
     * @return absl::Span<value_type>
     */
    absl::Span<value_type> getSpan(size_t channelIndex) const
    {
        ASSERT(channelIndex < numChannels)
        if (channelIndex < numChannels)
            return { buffers[channelIndex]->data(), buffers[channelIndex]->size() };

        return {};
    }

    /**
     * @brief Get a const Span object for a specific channel
     *
     * @param channelIndex
     * @return absl::Span<const value_type>
     */
    absl::Span<const value_type> getConstSpan(size_t channelIndex) const
    {
        return getSpan(channelIndex);
    }

    /**
     * @brief Add a channel to the buffer with the current number of frames.
     *
     */
    void addChannel()
    {
        if (numChannels < MaxChannels)
            buffers[numChannels++] = absl::make_unique<buffer_type>(numFrames);
    }

    /**
     * @brief Get the number of elements in each buffer
     *
     * @return size_t
     */
    size_t getNumFrames() const
    {
        return numFrames;
    }

    /**
     * @brief Get the number of channels
     *
     * @return int
     */
    size_t getNumChannels() const
    {
        return numChannels;
    }

    /**
     * @brief Check if the buffers contains no elements
     *
     * @return true
     * @return false
     */
    bool empty() const
    {
        return numFrames == 0;
    }

    /**
     * @brief Get a reference to a given element in a given buffer.
     *
     * In release builds this is not checked and may touch bad memory.
     *
     * @param channelIndex
     * @param frameIndex
     * @return Type&
     */
    Type& getSample(size_t channelIndex, size_t frameIndex)
    {
        // Uhoh
        ASSERT(buffers[channelIndex] != nullptr);
        ASSERT(frameIndex < numFrames);

        return *(buffers[channelIndex]->data() + frameIndex);
    }

    /**
     * @brief Alias for getSample(...)
     *
     * @param channelIndex
     * @param frameIndex
     * @return Type&
     */
    Type& operator()(size_t channelIndex, size_t frameIndex)
    {
        return getSample(channelIndex, frameIndex);
    }

    /**
     * @brief Remove all channels from the buffer and reset it to empty
     *
     */
    void reset()
    {
        for (size_t i = 0; i < numChannels; ++i)
            buffers[i].reset();
        numFrames = 0;
        numChannels = 0;
    }

    /**
     * @brief Add a positive number of channels to the buffer
     *
     * @param numChannels
     */
    void addChannels(size_t numChannels)
    {
        ASSERT(this->numChannels + numChannels <= MaxChannels);
        for (size_t i = 0; i < numChannels; ++i)
            addChannel();
    }

private:
    using buffer_type = Buffer<Type, Alignment>;
    using buffer_ptr = std::unique_ptr<buffer_type>;
    static_assert(MaxChannels > 0, "Need a positive number of channels");
    std::array<buffer_ptr, MaxChannels> buffers;
    size_t numChannels { 0 };
    size_t numFrames { 0 };
};
}
