#ifndef MIX_UTILS_MATH_UTILS
#define MIX_UTILS_MATH_UTILS

#include <type_traits>
#include <cstdint>

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
    inline auto two_pow (const N exponent) -> int64_t
    {
        return 1 << exponent;
    }
}

#endif