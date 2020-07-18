#ifndef _MIX_UTILS_MATH_UTILS_
#define _MIX_UTILS_MATH_UTILS_

#include <type_traits>
#include <cstdint>
#include <cstddef>


// TODO rename header to more_math.hpp
namespace mix::utils
{
    template<class Base, class Exponent>
    auto constexpr int_pow (Base base, Exponent exponent) -> Base
    {
        auto result = static_cast<Base>(1);

        for (;;)
        {
            if (exponent & 1)
            {
                result *= base;
            }

            exponent >>= 1;
            
            if (0 == exponent) break;
            
            base *= base;
        }

        return result;
    }
    
    template<class Exponent, class Result = uint64_t>
    auto constexpr two_pow (Exponent const exponent) -> Result
    {
        return static_cast<Result>(1) << exponent;
    }

    template<class N>
    auto constexpr is_power_of_two (N const num) -> bool
    {
        // TODO C++20 std::has_single_bit
        return !( (num - 1) & num );
    }
}

#endif