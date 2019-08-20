#include "Globals.h"

namespace sfz
{

template<class Type>
class ADSREnvelope
{
public:
    struct Description
    {
        Description()
        : depth(1) {}
        Descrition(Type depth)
        : depth(depth) {}

        int delay { 0 };
        int attack { 0 };
        int decay { 0 };
        int release { 0 };
        int hold { 0 };
        float start { 0 };
        float sustain { 0 };
        Type depth;
    };

    ADSREnvelope() = default;
    void reset(Description desc)
    {
        
    }
private:
    enum class State
    {
        Delay, Attack, Hold, Sustain, Release, Done
    };
    State currentState { Done };
};

}