#include "Globals.h"
template<class Type, unsigned int Alignment = SIMDConfig::defaultAlignment>
class InterleavedStereoBuffer
{
public:
    static constexpr int numChannels { 2 };
    InterleavedStereoBuffer() = default;
    InterleavedStereoBuffer(int numFrames)
    {
        resize(numFrames);
    }
    
    bool resize(int numFrames) 
    {
        // should have a positive number of frames...
        ASSERT(numFrames >= 0);
        if (buffer.resize(static_cast<size_t>(2*numFrames)))
        {
            this->numFrames = numFrames;
            return true;
        }
        return false;
    }
    
    struct Frame
    {
        Type left;
        Type right;
    };

    Frame& getFrame(int frameIndex)
    {
        return *static_cast<Frame*>(buffer.data() + 2 * frameIndex);
    }

    int getNumFrames() const noexcept { return numFrames; }
    int getNumChannels() const noexcept { return numChannels; }
    bool empty() const noexcept { return numFrames == 0; }
private:
    static constexpr auto TypeAlignment { Alignment / sizeof(Type) };
    static constexpr auto TypeAlignmentMask { TypeAlignment - 1 };
    static_assert(TypeAlignment * sizeof(Type) == Alignment, "The alignment does not appear to be divided by the size of the Type");
    int numFrames { 0 };
    int totalSize { 0 };
    int padding { 0 };
    Buffer<Type, Alignment> buffer {};
    Type trash { 0 };
};