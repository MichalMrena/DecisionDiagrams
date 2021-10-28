#ifndef MIX_DD_UTILS_HPP
#define MIX_DD_UTILS_HPP

#include "types.hpp"
#include <concepts>
#include <functional>
#include <ranges>
#include <vector>

namespace teddy::utils
{
    template<class F>
    concept i_gen = requires (F f, uint_t k)
    {
        std::invoke(f, k);
    };

    template<i_gen Gen>
    auto fill_vector (std::size_t const n, Gen&& f)
    {
        using T = decltype(std::invoke(f, uint_t {}));
        auto xs = std::vector<T>();
        xs.reserve(n);
        for (auto i = uint_t {0}; i < n; ++i)
        {
            xs.emplace_back(std::invoke(f, i));
        }
        return xs;
    }

    template<std::ranges::input_range Xs, class F>
    auto fmap (Xs&& xs, F&& f)
    {
        namespace rs = std::ranges;
        using U = decltype(std::invoke(f, *rs::begin(xs)));
        auto ys = std::vector<U>();
        if constexpr (std::ranges::sized_range<Xs>)
        {
            ys.reserve(rs::size(xs));
        }
        for (auto it = rs::begin(xs); it != rs::end(xs); ++it)
        {
            auto&& x = *it;
            ys.emplace_back(std::invoke(f, std::forward<decltype(x)>(x)));
        }
        return ys;
    }

    auto constexpr identity = [](auto const a) { return a; };

    template<class Base, std::integral Exponent>
    auto constexpr int_pow (Base base, Exponent exponent) -> Base
    {
        auto result = Base {1};

        for (;;)
        {
            if (exponent & 1)
            {
                result *= base;
            }

            exponent >>= 1;

            if (0 == exponent)
            {
                break;
            }

            base *= base;
        }

        return result;
    }
}

#endif