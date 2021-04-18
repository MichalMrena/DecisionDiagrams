#ifndef MIX_UTILS_MATH_UTILS_HPP
#define MIX_UTILS_MATH_UTILS_HPP

namespace teddy::utils
{
    /**
     *  @brief Exponentiation for integral types.
     */
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

    /**
     *  @brief Calculates power of two for integral types.
     */
    template<class Exponent, class Result = std::uint64_t>
    auto constexpr two_pow (Exponent const exponent) -> Result
    {
        return static_cast<Result>(1) << exponent;
    }

    /**
     *  @brief Check if @p num is power of two.
     */
    template<class N>
    auto constexpr is_power_of_two (N const num) -> bool
    {
        return num && !((num - 1) & num);
    }
}

#endif