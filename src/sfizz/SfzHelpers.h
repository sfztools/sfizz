// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <absl/types/optional.h>
#include <absl/strings/string_view.h>
//#include <string>
#include <array>
#include <cmath>
#include "Macros.h"
#include "Config.h"
#include "MathHelpers.h"
#include "SIMDHelpers.h"
#include "absl/meta/type_traits.h"
#include "Defaults.h"

namespace sfz
{

using CCNamePair = std::pair<uint16_t, std::string>;
template<class T>
using MidiNoteArray = std::array<T, 128>;
template<class ValueType>
struct CCValuePair {
    int cc;
    ValueType value;
};

template<class ValueType, bool CompareValue = false>
struct CCValuePairComparator {
    bool operator()(const CCValuePair<ValueType>& valuePair, const int& cc)
    {
        return (valuePair.cc < cc);
    }

    bool operator()(const int& cc, const CCValuePair<ValueType>& valuePair)
    {
        return (cc < valuePair.cc);
    }

    bool operator()(const CCValuePair<ValueType>& lhs, const CCValuePair<ValueType>& rhs)
    {
        return (lhs.cc < rhs.cc);
    }
};

template<class ValueType>
struct CCValuePairComparator<ValueType, true> {
    bool operator()(const CCValuePair<ValueType>& valuePair, const ValueType& value)
    {
        return (valuePair.value < value);
    }

    bool operator()(const ValueType& value, const CCValuePair<ValueType>& valuePair)
    {
        return (value < valuePair.value);
    }

    bool operator()(const CCValuePair<ValueType>& lhs, const CCValuePair<ValueType>& rhs)
    {
        return (lhs.value < rhs.value);
    }
};

struct MidiEvent {
    int delay;
    float value;
};
using EventVector = std::vector<MidiEvent>;

struct MidiEventDelayComparator {
    bool operator()(const MidiEvent& event, const int& delay)
    {
        return (event.delay < delay);
    }

    bool operator()(const int& delay, const MidiEvent& event)
    {
        return (delay < event.delay);
    }

    bool operator()(const MidiEvent& lhs, const MidiEvent& rhs)
    {
        return (lhs.delay < rhs.delay);
    }
};

struct MidiEventValueComparator {
    bool operator()(const MidiEvent& event, const float& value)
    {
        return (event.value < value);
    }

    bool operator()(const float& value, const MidiEvent& event)
    {
        return (value < event.value);
    }

    bool operator()(const MidiEvent& lhs, const MidiEvent& rhs)
    {
        return (lhs.value < rhs.value);
    }
};

/**
 * @brief Converts cents to a pitch ratio
 *
 * @tparam T
 * @param cents
 * @param centsPerOctave
 * @return constexpr float
 */
template<class T>
constexpr float centsFactor(T cents, T centsPerOctave = 1200)
{
    return std::pow(2.0f, static_cast<float>(cents) / centsPerOctave);
}

template<class T, absl::enable_if_t<std::is_integral<T>::value, int> = 0>
constexpr T denormalize7Bits(float value)
{
    return static_cast<T>(value * 127.0f);
}

constexpr uint8_t denormalizeCC(float value)
{
    return denormalize7Bits<uint8_t>(value);
}

constexpr uint8_t denormalizeVelocity(float value)
{
    return denormalize7Bits<uint8_t>(value);
}

template<class T>
constexpr float normalize7Bits(T value)
{
    return static_cast<float>(min(max(value, T{ 0 }), T{ 127 })) / 127.0f;
}

/**
 * @brief Normalize a CC value between 0.0 and 1.0
 *
 * @tparam T
 * @param ccValue
 * @return constexpr float
 */
template<class T>
constexpr float normalizeCC(T ccValue)
{
    return normalize7Bits(ccValue);
}

/**
 * @brief Normalize a velocity between 0.0 and 1.0
 *
 * @tparam T
 * @param ccValue
 * @return constexpr float
 */
template<class T>
constexpr float normalizeVelocity(T velocity)
{
    return normalize7Bits(velocity);
}


/**
 * @brief Normalize a percentage between 0 and 1
 *
 * @tparam T
 * @param percentValue
 * @return constexpr float
 */
template<class T>
constexpr float normalizePercents(T percentValue)
{
    return percentValue * 0.01f;
}

/**
 * @brief Normalize bends between -1 and 1. We clamp to 8191 instead of 8192 in the low end
 * to have something symmetric with respect to 0.
 *
 * @param bendValue
 * @return constexpr float
 */
constexpr float normalizeBend(float bendValue)
{
    return clamp(bendValue, -8191.0f, 8191.0f) / 8191.0f;
}

namespace literals
{
inline float operator ""_norm(unsigned long long int value)
{
    if (value > 127)
        value = 127;

    return normalize7Bits(value);
}
}


/**
 * @brief Convert a note in string to its equivalent midi note number
 *
 * @param value
 * @return absl::optional<uint8_t>
 */
absl::optional<uint8_t> readNoteValue(const absl::string_view& value);

/**
 * @brief From a source view, find the next sfz header and its members and
 *          return them, while updating the source by removing this header
 *          and members from the beginning. The function "consumes" the
 *          header and its members from the source if found.
 *
 * No check is made to see if the header is "valid" in the sfz sense.
 * The output parameters are set only if the method returns true.
 *
 * @param source A source view; can be updated and shortened
 * @param header An output view on the header, without the <>
 * @param members An output view on the members, untrimmed
 * @return true if a header was found
 * @return false otherwise
 */
bool findHeader(absl::string_view& source, absl::string_view& header, absl::string_view& members);
/**
 * @brief From a source view, find the next sfz member opcode and its value.
 *          Return them while updating the source by removing this opcode
 *          and value from the beginning. The function "consumes" the
 *          opcode from the source if one is found.
 *
 * No check is made to see if the opcode is "valid" in the sfz sense.
 * The output parameters are set only if the method returns true.
 *
 * @param source A source view; can be updated and shortened
 * @param opcode An output view on the opcode name
 * @param value An output view on the opcode value
 * @return true if an opcode was found
 * @return false
 */
bool findOpcode(absl::string_view& source, absl::string_view& opcode, absl::string_view& value);

/**
 * @brief Find an SFZ #define statement on a line and return the variable and value as views.
 *
 * This function assums that there is a single define per line and that the variable and value
 * are separated by whitespace.
 * The output parameters are set only if the method returns true.
 *
 * @param line The source line
 * @param variable An output view on the define variable
 * @param value An output view on the define value
 * @return true If a define was found
 * @return false
 */
bool findDefine(absl::string_view line, absl::string_view& variable, absl::string_view& value);

/**
 * @brief Find an SFZ #include statement on a line and return included path.
 *
 * This function assums that there is a single include per line and that the
 * include path is within quotes.
 * The output parameter is set only if the method returns true.
 *
 * @param line The source line
 * @param path The path, if found
 * @return true If an include was found
 * @return false
 */
bool findInclude(absl::string_view line, std::string& path);


/**
 * @brief multiply a value by a factor, in cents. To be used for pitch variations.
 *
 * @param base
 * @param modifier
 */
inline CXX14_CONSTEXPR float multiplyByCentsModifier(int modifier, float base)
{
   return base * centsFactor(modifier);
}

template<class T>
inline CXX14_CONSTEXPR float gainModifier(T modifier, float value)
{
    return value * modifier;
}


/**
 * @brief Compute a crossfade in value with respect to a crossfade range (note, velocity, cc, ...)
 */
template<class T, class U>
float crossfadeIn(const sfz::Range<T>& crossfadeRange, U value, SfzCrossfadeCurve curve)
{
    if (value < crossfadeRange.getStart())
        return 0.0f;

    const auto length = static_cast<float>(crossfadeRange.length());
    if (length == 0.0f)
        return 1.0f;

    else if (value < crossfadeRange.getEnd()) {
        const auto crossfadePosition = static_cast<float>(value - crossfadeRange.getStart()) / length;
        if (curve == SfzCrossfadeCurve::power)
            return sqrt(crossfadePosition);
        if (curve == SfzCrossfadeCurve::gain)
            return crossfadePosition;
    }

    return 1.0f;
}


/**
 * @brief Compute a crossfade out value with respect to a crossfade range (note, velocity, cc, ...)
 */
template<class T, class U>
float crossfadeOut(const sfz::Range<T>& crossfadeRange, U value, SfzCrossfadeCurve curve)
{
    if (value > crossfadeRange.getEnd())
        return 0.0f;

    const auto length = static_cast<float>(crossfadeRange.length());
    if (length == 0.0f)
        return 1.0f;

    else if (value > crossfadeRange.getStart()) {
        const auto crossfadePosition = static_cast<float>(value - crossfadeRange.getStart()) / length;
        if (curve == SfzCrossfadeCurve::power)
            return std::sqrt(1 - crossfadePosition);
        if (curve == SfzCrossfadeCurve::gain)
            return 1 - crossfadePosition;
    }

    return 1.0f;
}

template<class F>
void linearEnvelope(const EventVector& events, absl::Span<float> envelope, F&& lambda)
{
    ASSERT(events.size() > 0);
    ASSERT(events[0].delay == 0);

    auto lastValue = lambda(events[0].value);
    auto lastDelay = events[0].delay;
    for (unsigned i = 1; i < events.size(); ++ i) {
        const auto event = events[i];
        const auto length = event.delay - lastDelay;
        const auto step = (lambda(event.value) - lastValue)/ length;
        lastValue = linearRamp<float>(envelope.subspan(lastDelay, length), lastValue, step);
        lastDelay += length;
    }
    fill<float>(envelope.subspan(lastDelay), lastValue);
}

} // namespace sfz

