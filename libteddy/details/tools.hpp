#ifndef LIBTEDDY_DETAILS_UTILS_HPP
#define LIBTEDDY_DETAILS_UTILS_HPP

#include <libteddy/details/types.hpp>

#include <charconv>
#include <concepts>
#include <functional>
#include <optional>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace teddy::utils
{
template<class Gen>
concept i_gen           = requires(Gen generator, int32 index)
{
    std::invoke(generator, index);
};

template<class T>
concept is_std_vector = std::same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>;

/**
 *  \brief Identity function
 */
auto constexpr identity = [] (auto const arg)
{
    return arg;
};

/**
 *  \brief Checks if argument is not zerp
 */
auto constexpr not_zero = [] (auto const arg)
{
    return arg != 0;
};

/**
 *  \brief Creates constant function
 */
auto constexpr constant = [] (auto const arg)
{
    return [arg] (auto)
    {
        return arg;
    };
};

template<i_gen Gen>
auto fill_vector (int64 const n, Gen generator)
{
    using T = decltype(std::invoke(generator, int32 {}));
    auto data = std::vector<T>();
    data.reserve(as_usize(n));
    for (auto i = int32 {0}; i < n; ++i)
    {
        data.emplace_back(std::invoke(generator, i));
    }
    return data;
}

template<std::input_iterator I, std::sentinel_for<I> S, class F>
auto fmap (I first, S last, F mapper)
{
    using U = decltype(std::invoke(mapper, *first));
    auto result = std::vector<U>();
    if constexpr (std::random_access_iterator<I>)
    {
        result.reserve(as_usize(std::distance(first, last)));
    }
    while (first != last)
    {
        result.emplace_back(std::invoke(mapper, *first));
        ++first;
    }
    return result;
}

template<std::ranges::input_range Range, class F>
auto fmap (Range&& input, F mapper)
{
    return fmap(std::ranges::begin(input), std::ranges::end(input), mapper);
}

/**
 *  \brief Exponentiation for integers
 */
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

/**
 *  \brief Tries to parse \p input to \p Num
 *  \param input input string
 *  \return optinal result
 */
template<class Num>
auto parse (std::string_view const input) -> std::optional<Num>
{
    auto ret    = Num {};
    auto result = std::from_chars(input.data(), input.data() + input.size(), ret);
    return std::errc {} == result.ec && result.ptr == input.data() + input.size()
             ? std::optional<Num>(ret)
             : std::nullopt;
}

/**
 *  \brief Function object for tuple hash
 */
struct tuple_hash
{
template<class... Ts>
auto operator()(std::tuple<Ts...> const& tuple) const noexcept
{
    // see boost::hash_combine
    auto seed = std::size_t{0};
    auto hash_combine = [&seed](auto const& elem)
    {
        auto hasher = std::hash<std::remove_cvref_t<decltype(elem)>>();
        seed ^= hasher(elem) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return std::tuple<>();
    };
    std::apply(
        [&](auto const&... elems){ return (hash_combine(elems), ...); },
        tuple
    );
    return seed;
}
};

/**
 *  \brief Checks if any of the arguments is true
 *  \param args boolean arguments
 *  \return true if any of the arguments is true
 */
template<class... Args>
auto any (Args... args)
{
    return (args || ...);
}
} // namespace teddy::utils

#endif