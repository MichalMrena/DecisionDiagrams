#ifndef MIX_UTILS_MATH_UTILS
#define MIX_UTILS_MATH_UTILS

#include <type_traits>

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
}

#endif