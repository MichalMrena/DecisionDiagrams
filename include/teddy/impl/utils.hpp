#ifndef MIX_DD_UTILS_HPP
#define MIX_DD_UTILS_HPP

#include "types.hpp"
#include <charconv>
#include <concepts>
#include <functional>
#include <optional>
#include <ranges>
#include <vector>

namespace teddy::utils
{
    template<class F>
    concept i_gen = requires (F f, uint_t k)
    {
        std::invoke(f, k);
    };

    auto constexpr identity = [](auto const a) { return a; };

    auto constexpr not_zero = [](auto const x) { return x != 0; };

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

    template<std::input_iterator I, std::sentinel_for<I> S, class F>
    auto fmap (I it, S last, F f)
    {
        using U = decltype(std::invoke(f, *it));
        auto ys = std::vector<U>();
        if constexpr (std::random_access_iterator<I>)
        {
            ys.reserve(static_cast<std::size_t>(
                std::ranges::distance(it, last))
            );
        }
        while (it != last)
        {
            ys.emplace_back(std::invoke(f, *it));
            ++it;
        }
        return ys;
    }

    template<std::ranges::input_range Xs, class F>
    auto fmap (Xs&& xs, F f)
    {
        return fmap(std::ranges::begin(xs), std::ranges::end(xs), f);
    }

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

    template<class Num>
    auto parse (std::string_view const in) -> std::optional<Num>
    {
        auto ret    = Num {};
        auto result = std::from_chars(in.data(), in.data() + in.size(), ret);
        return std::errc {} == result.ec && result.ptr == in.data() + in.size()
            ? std::optional<Num>(ret)
            : std::nullopt;
    }
}

#endif