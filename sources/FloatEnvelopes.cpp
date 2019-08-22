#include "LinearEnvelope.h"
#include "ADSREnvelope.h"

// Include the generic implementations
#include "LinearEnvelope.cpp"
#include "ADSREnvelope.cpp"

// And explicitely instantiate the float version
namespace sfz
{
    template class LinearEnvelope<float>;
    template class ADSREnvelope<float>;
}