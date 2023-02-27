#ifndef TEDDY_SRC_UTILS_HPP
#define TEDDY_SRC_UTILS_HPP

#include <algorithm>
#include <bits/ranges_cmp.h>
#include <functional>
#include <libteddy/details/types.hpp>
#include <libteddy/details/utils.hpp>
#include <tuple>
#include <vector>

#define otherwise

using teddy::int64;

/**
 *  @brief Marks unreachable part of control flow that is a logic error.
 */
inline auto unreachable() -> void
{
    std::terminate();
}

/**
 * @brief Result of @c group .
 */
template<class T>
struct Group
{
    T elem_;
    int64 count_;
};

/**
 *  @brief Groups ordered @p xs by value.
 *  @tparam T data type
 *  @tparam F group by projection function
 */
template<class T, class F>
auto group_by (std::vector<T> const& xs, F f) -> std::vector<Group<T>>
{
    using U = decltype(f(T()));

    auto pairs = std::vector<std::tuple<T const*, U>>();
    for (auto const& x : xs)
    {
        pairs.emplace_back(&x, std::invoke(f, x));
    }

    std::ranges::sort(pairs, std::ranges::less(), [](auto const& p)
    {
        return std::get<1>(p);
    });

    auto groups = std::vector<Group<T>>();
    auto it = begin(pairs);
    auto const last = end(pairs);
    while (it != last)
    {
        auto const [elemptr, elemproj] = *it;
        auto& group = groups.emplace_back(Group<T>{*elemptr, 0});
        while (it != last && std::get<1>(*it) == elemproj)
        {
            ++it;
            ++group.count_;
        }
    }
    return groups;
}

/**
 *  @brief Groups ordered @p xs by value.
 */
template<class T>
auto group (std::vector<T> const& xs) -> std::vector<Group<T>>
{
    return group_by(xs, teddy::utils::identity);
}

template<class Int>
auto factorial (Int n) -> Int
{
    auto result = Int{1};
    while (n > 1)
    {
        result *= n;
        --n;
    }
    return result;
}

template<class Int>
auto n_over_k (Int const n, Int const k) -> Int
{
    return
        k == 0 ?
            1 :
        k == 1 ?
            n :
        k > n / 2 ?
            n_over_k<Int>(n, n - k) :
        otherwise
            Int{n * n_over_k<Int>(n - 1, k - 1) / k};
}

template<class Int>
auto combin_r (Int const n, Int const k) -> Int
{
    return n_over_k<Int>(n + k - 1, k);
}

#endif