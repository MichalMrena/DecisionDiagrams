#ifndef LIBTEDDY_DETAILS_UTILS_HPP
#define LIBTEDDY_DETAILS_UTILS_HPP

#include <libteddy/details/types.hpp>

#include <charconv>
#include <concepts>
#include <functional>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace teddy::utils
{
template<class T>
concept is_std_vector = std::
    same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>;

template<class Gen>
auto fill_vector (int64 const n, Gen generator)
{
    using T   = decltype(std::invoke(generator, int32 {}));
    auto data = std::vector<T>();
    data.reserve(as_usize(n));
    for (auto i = int32 {0}; i < n; ++i)
    {
        data.emplace_back(std::invoke(generator, i));
    }
    return data;
}

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

/**
 *  \brief Eats everything, does nothing
 */
auto constexpr no_op = [] (auto const&...)
{
};

/**
 *  \brief Exponentiation by squaring
 */
template<class Base>
auto constexpr int_pow(Base base, int32 exponent) -> Base
{
    Base result = 1;

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
    auto ret = Num {};
    auto result
        = std::from_chars(input.data(), input.data() + input.size(), ret);
    return std::errc {} == result.ec
                && result.ptr == input.data() + input.size()
             ? std::optional<Num>(ret)
             : std::nullopt;
}

/**
 *  \brief Computes hash of the \p args
 */
template<class... Ts>
auto hash_combine (Ts const&... args) -> std::size_t
{
    // see boost::hash_combine
    std::size_t seed = 0;
    auto combine = [&seed] (auto const& elem)
    {
        auto hasher = std::hash<std::remove_cvref_t<decltype(elem)>>();
        seed ^= hasher(elem) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    };
    (combine(args), ...);
    return seed;
}

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

/**
 *  \brief The min function
 */
template<class T>
constexpr auto min(T lhs, T rhs) -> T
{
    return lhs < rhs ? lhs : rhs;
}

/**
 *  \brief The min function for parameter packs
 */
template<class X>
constexpr auto pack_min (X x) -> X
{
    return x;
}

/**
 *  \brief The min function for parameter packs
 */
template<class X, class... Xs>
constexpr auto pack_min (X x, Xs... xs) -> X
{
    return min(x, pack_min(xs...));
}

/**
 *  \brief Exchages value of \p var to \p newVal and returns the old value
 *  Simplified implementation of std::exchange
 */
template<class T, class U = T>
auto exchange(T& var, U newVal) noexcept -> T
{
    auto oldVal = var;
    var = newVal;
    return oldVal;
}

/**
 *  \brief Swaps values in \p first and \p second
 *  Simplified implementation of std::swap
 */
template<typename T>
constexpr auto swap(T& first, T& second) noexcept -> void
{
	auto tmp = first;
	first = second;
	second = tmp;
}

// TODO remove type_traits
template<class X, class T>
using second_t = std::conditional_t<false, X, T>;

template<class T>
struct is_void
{
    static constexpr bool value = false;
};

template<>
struct is_void<void>
{
    static constexpr bool value = true;
};

template<class T, class U>
struct is_same
{
    static constexpr bool value = false;
};

template<class T>
struct is_same<T, T>
{
    static constexpr bool value = true;
};

template<class T, class U>
concept same_as = std::is_same<T, U>::value;

template<class T>
struct optional_member
{
    T member_;
};

template<>
struct optional_member<void>
{
};
} // namespace teddy::utils


#endif