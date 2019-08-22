#pragma once
#include "Globals.h"
#include <cmath>
#include <absl/types/span.h>

template<class Type=float>
class OnePoleFilter
{
public:
    OnePoleFilter() = default;
    // Normalized cutoff with respect to the sampling rate
    template<class C>
    static Type normalizedGain(Type cutoff, C sampleRate)
    {
        return std::tan( cutoff / static_cast<Type>(sampleRate) * M_PIf32 );
    }
    
    OnePoleFilter(Type gain)
    {
        setGain(gain);
    }

    void setGain(Type gain)
    {
        this->gain = gain;
        G = gain / ( 1 + gain);
    }

    Type getGain() const { return gain; }

    int processLowpass(absl::Span<const Type> input, absl::Span<Type> lowpass)
    {
        for (auto [in, out] = std::pair(input.begin(), lowpass.begin()); 
            in < input.end() && out < lowpass.end(); in++, out++)
        {
            oneLowpass(in, out);
        }
        return std::min(input.size(), lowpass.size());
    }

    int processHighpass(absl::Span<const Type> input, absl::Span<Type> highpass)
    {
        for (auto [in, out] = std::pair(input.begin(), highpass.begin()); 
            in < input.end() && out < highpass.end(); in++, out++)
        {
            oneHighpass(in, out);
        }
        return std::min(input.size(), highpass.size());
    }

    int processLowpassVariableGain(absl::Span<const Type> input, absl::Span<Type> lowpass, absl::Span<const Type> gain)
    {
        for (auto [in, out, g] = std::tuple(input.begin(), lowpass.begin(), gain.begin()); 
            in < input.end() && out < lowpass.end() && g < gain.end(); in++, out++, g++)
        {
            setGain(*g);
            oneLowpass(in, out);
        }
        
        return std::min({ input.size(), lowpass.size(), gain.size() });
    }

    int processHighpassVariableGain(absl::Span<const Type> input, absl::Span<Type> highpass, absl::Span<const Type> gain)
    {
        for (auto [in, out, g] = std::tuple(input.begin(), highpass.begin(), gain.begin()); 
            in < input.end() && out < highpass.end() && g < gain.end(); in++, out++, g++)
        {
            setGain(*g);
            oneHighpass(in, out);
        }
        
        return std::min({ input.size(), highpass.size(), gain.size() });
    }

    void reset() { state = 0.0; }
private:
    Type state { 0.0 };
    Type gain { 0.25 };
    Type intermediate { 0.0 };
    Type G { gain / (1 + gain) };
    
    inline void oneLowpass(const Type* in, Type* out)
    {
        intermediate = G * (*in - state);
        *out = intermediate + state;
        state = *out + intermediate;
    }

    inline void oneHighpass(const Type* in, Type* out)
    {
        intermediate = G * (*in - state);
        *out = *in - intermediate - state;
        state += 2*intermediate;
    }
};