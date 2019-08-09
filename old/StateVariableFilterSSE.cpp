#include "StateVariableFilter.h"
#include <tuple>
#include "Intrinsics.h"

// template<>
// template<>
// auto StateVariableFilter<SVFReturn::BP>::process<__m128>(const __m128& input [[maybe_unused]])
// {
//     return 0.0;
// }

// template<>
// template<>
// auto StateVariableFilter<SVFReturn::BP_LP>::process<__m128>(const __m128& input [[maybe_unused]])
// {
//     return std::make_pair<float, float>(0.0, 0.0);
// }

// template<>
// template<>
// auto StateVariableFilter<SVFReturn::BP_LP_HP>::process<__m128>(const __m128& input [[maybe_unused]])
// {
//     return std::make_tuple<float, float, float>(0.0, 0.0, 0.0);
// }

template<>
auto StateVariableFilter::process(const __m128& input [[maybe_unused]], __m128& bandpass [[maybe_unused]]) 
{

}

template<>
auto StateVariableFilter::process(const __m128& input [[maybe_unused]], __m128& bandpass [[maybe_unused]], __m128& lowpass [[maybe_unused]]) 
{

}