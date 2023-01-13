#ifndef LIBTEDDY_DETAILS_UTILS_HPP
#define LIBTEDDY_DETAILS_UTILS_HPP

#include <charconv>
#include <concepts>
#include <functional>
#include <libteddy/details/types.hpp>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

namespace teddy::utils
{
template<class F>
concept i_gen           = requires(F f, int32 k) { std::invoke(f, k); };

auto constexpr identity = [](auto const a)
{
    return a;
};

auto constexpr not_zero = [](auto const x)
{
    return x != 0;
};

auto constexpr constant = [](auto const c)
{
    return [c](auto)
    {
        return c;
    };
};

auto constexpr fix = [](auto f)
{
    return [f](auto&&... args)
    {
        return f(f, std::forward<decltype(args)>(args)...);
    };
};

template<i_gen Gen>
auto fill_vector(int64 const n, Gen&& f)
{
    using T = decltype(std::invoke(f, int32 {}));
    auto xs = std::vector<T>();
    xs.reserve(as_usize(n));
    for (auto i = int32 {0}; i < n; ++i)
    {
        xs.emplace_back(std::invoke(f, i));
    }
    return xs;
}

template<std::input_iterator I, std::sentinel_for<I> S, class F>
auto fmap(I it, S last, F f)
{
    using U = decltype(std::invoke(f, *it));
    auto ys = std::vector<U>();
    if constexpr (std::random_access_iterator<I>)
    {
        ys.reserve(as_usize(std::distance(it, last)));
    }
    while (it != last)
    {
        ys.emplace_back(std::invoke(f, *it));
        ++it;
    }
    return ys;
}

template<std::ranges::input_range Xs, class F>
auto fmap(Xs&& xs, F f)
{
    return fmap(std::ranges::begin(xs), std::ranges::end(xs), f);
}

template<class Base, std::integral Exponent>
auto constexpr int_pow(Base base, Exponent exponent) -> Base
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
auto parse(std::string_view const in) -> std::optional<Num>
{
    auto ret    = Num {};
    auto result = std::from_chars(in.data(), in.data() + in.size(), ret);
    return std::errc {} == result.ec && result.ptr == in.data() + in.size()
               ? std::optional<Num>(ret)
               : std::nullopt;
}
} // namespace teddy::utils

#endif