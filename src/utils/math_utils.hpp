#ifndef _MIX_UTILS_MATH_UTILS_
#define _MIX_UTILS_MATH_UTILS_

#include <type_traits>
#include <cstdint>
#include <cstddef>

namespace mix::utils
{
    template<class N1
           , class N2
           , typename std::enable_if<std::is_integral<N2>::value, N2>::type* = nullptr>
    constexpr auto pow (N1 base, N2 exponent) -> N1
    {
        N1 result {1};

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

    template<class N
           , typename std::enable_if<std::is_integral<N>::value, N>::type* = nullptr>
    constexpr auto two_pow (const N exponent) -> int64_t
    {
        return 1 << exponent;
    }

    template<size_t N>
    struct is_power_of_two
    {
        static constexpr bool value {!((N - 1) & N)};
    };
}

#endif