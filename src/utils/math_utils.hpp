#ifndef _MIX_UTILS_MATH_UTILS_
#define _MIX_UTILS_MATH_UTILS_

#include <type_traits>
#include <cstdint>
#include <cstddef>

namespace mix::utils
{
    template<class N>
    inline constexpr auto is_signed_integral_v
    {
        std::integral_constant< bool
                              , std::is_integral<N>::value 
                                && std::is_signed<N>::value>::value
    };
    
    template<class N>
    inline constexpr auto is_unsigned_integral_v
    {
        std::integral_constant< bool
                              , std::is_integral<N>::value 
                                && std::is_unsigned<N>::value>::value
    };

    template< class BaseT
            , class ExponentT
            , class = typename std::enable_if<std::is_integral<ExponentT>::value, ExponentT>::type >
    constexpr auto pow (BaseT base, ExponentT exponent) -> BaseT
    {
        BaseT result {1};

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
    
    template< class N
            , class = typename std::enable_if<std::is_integral<N>::value, N>::type >
    constexpr auto two_pow (const N exponent) -> uint64_t
    {
        return static_cast<uint64_t>(1) << exponent;
    }

    template< class N
            , class = typename std::enable_if<std::is_integral<N>::value, N>::type >
    constexpr auto is_power_of_two (const N num) -> bool
    {
        return !( (num - 1) & num );
    }
}

#endif