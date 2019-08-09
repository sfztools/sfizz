#include "StateVariableFilter.h"
#include <tuple>

// template<>
// template<>
// auto StateVariableFilter<SVFReturn::BP>::process<float>(const float& input [[maybe_unused]])
// {
//     return 0.0;
// }

// template<>
// template<>
// auto StateVariableFilter<SVFReturn::BP_LP>::process<float>(const float& input [[maybe_unused]])
// {
//     return std::make_pair<float, float>(0.0, 0.0);
// }

// template<>
// template<>
// auto StateVariableFilter<SVFReturn::BP_LP_HP>::process<float>(const float& input [[maybe_unused]])
// {
//     return std::make_tuple<float, float, float>(0.0, 0.0, 0.0);
// }

template<>
auto StateVariableFilter::process(const float& input [[maybe_unused]], float& bandpass [[maybe_unused]]) 
{

}

template<>
auto StateVariableFilter::process(const float& input [[maybe_unused]], float& bandpass [[maybe_unused]], float& lowpass [[maybe_unused]]) 
{
    
}