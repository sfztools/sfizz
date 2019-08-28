#pragma once
#include "Helpers.h"
#include "SIMDHelpers.h"
#include "StereoBuffer.h"
#include <absl/types/span.h>
#include <type_traits>

template <class Type>
class StereoSpan {
public:
    using ValueType = std::remove_cv_t<Type>;
    template <typename U>
    StereoSpan() = delete;
    StereoSpan(Type* leftBuffer, Type* rightBuffer, size_t numFrames)
        : numFrames(numFrames)
        , leftBuffer(leftBuffer, numFrames)
        , rightBuffer(rightBuffer, numFrames)
    {
    }

    StereoSpan(void* leftBuffer, void* rightBuffer, size_t numFrames)
        : numFrames(numFrames)
        , leftBuffer(static_cast<Type*>(leftBuffer), numFrames)
        , rightBuffer(static_cast<Type*>(rightBuffer), numFrames)
    {
    }

    StereoSpan(absl::Span<Type> leftBuffer, absl::Span<Type> rightBuffer)
        : numFrames(std::min(leftBuffer.size(), rightBuffer.size()))
        , leftBuffer(leftBuffer.first(numFrames))
        , rightBuffer(rightBuffer.first(numFrames))
    {
        // Buffer really should be the same size here
        ASSERT(leftBuffer.size() == rightBuffer.size());
    }

    StereoSpan(StereoBuffer<const ValueType>& buffer)
        : leftBuffer(buffer.getConstSpan(Channel::left))
        , rightBuffer(buffer.getConstSpan(Channel::right))
        , numFrames(buffer.getNumFrames())
    {
    }

    StereoSpan(StereoBuffer<ValueType>& buffer)
        : numFrames(buffer.getNumFrames())
        , leftBuffer(buffer.getSpan(Channel::left))
        , rightBuffer(buffer.getSpan(Channel::right))
    {
    }

    StereoSpan(StereoBuffer<ValueType>& buffer, size_t numFrames)
        : numFrames(numFrames)
        , leftBuffer(buffer.getSpan(Channel::left).first(numFrames))
        , rightBuffer(buffer.getSpan(Channel::right).first(numFrames))
    {
    }

    StereoSpan(const StereoBuffer<const ValueType>& buffer, size_t numFrames)
        : numFrames(numFrames)
        , leftBuffer(buffer.getConstSpan(Channel::left).first(numFrames))
        , rightBuffer(buffer.getConstSpan(Channel::right).first(numFrames))
    {
    }

    StereoSpan(const StereoSpan<ValueType>& span)
        : numFrames(span.size())
        , leftBuffer(span.left())
        , rightBuffer(span.right())
    {
    }

    StereoSpan(const StereoSpan<const ValueType>& span)
        : numFrames(span.size())
        , leftBuffer(span.left())
        , rightBuffer(span.right())
    {
    }

    void fill(Type value) noexcept
    {
        ::fill<Type>(leftBuffer, value);
        ::fill<Type>(rightBuffer, value);
    }

    void applyGain(absl::Span<const Type> gain) noexcept
    {
        ::applyGain<Type>(gain, leftBuffer);
        ::applyGain<Type>(gain, rightBuffer);
    }

    void applyGain(Type gain) noexcept
    {
        ::applyGain<Type>(gain, leftBuffer);
        ::applyGain<Type>(gain, rightBuffer);
    }

    void readInterleaved(absl::Span<const Type> input) noexcept
    {
        ASSERT(input.size() <= static_cast<size_t>(numChannels * numFrames));
        ::readInterleaved<Type>(input, absl::MakeSpan(leftBuffer), absl::MakeSpan(rightBuffer));
    }

    void writeInterleaved(absl::Span<Type> output) noexcept
    {
        ASSERT(output.size() >= static_cast<size_t>(numChannels * numFrames));
        ::writeInterleaved<Type>(leftBuffer, rightBuffer, output);
    }

    void add(StereoSpan<const Type> buffer)
    {
        ::add<Type>(buffer.left(), leftBuffer);
        ::add<Type>(buffer.right(), rightBuffer);
    }

    absl::Span<Type> left() const
    {
        return leftBuffer;
    }

    absl::Span<Type> right() const
    {
        return rightBuffer;
    }

    StereoSpan<Type> first(size_t length) const
    {
        return { leftBuffer.first(length), rightBuffer.first(length) };
    }

    StereoSpan<Type> last(size_t length) const
    {
        return { leftBuffer.last(length), rightBuffer.last(length) };
    }

    StereoSpan<Type> subspan(size_t pos, size_t length = absl::Span<Type>::npos) const
    {
        return { leftBuffer.subspan(pos, length), rightBuffer.subspan(pos, length) };
    }

    size_t size() const
    {
        return numFrames;
    }

private:
    static constexpr int numChannels { 2 };
    size_t numFrames { 0 };
    absl::Span<Type> leftBuffer;
    absl::Span<Type> rightBuffer;
    LEAK_DETECTOR(StereoSpan);
};