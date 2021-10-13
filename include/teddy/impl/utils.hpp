#ifndef MIX_DD_UTILS_HPP
#define MIX_DD_UTILS_HPP

#include "types.hpp"
#include <functional>
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

    template<class Xs, class F>
    auto fmap (Xs&& xs, F&& f)
    {
        using U = decltype(std::invoke(f, *std::begin(xs)));
        auto ys = std::vector<U>();
        ys.reserve(std::size(xs));
        for (auto&& x : xs)
        {
            ys.emplace_back(std::invoke(f, std::forward<decltype(x)>(x)));
        }
        return ys;
    }

    auto constexpr identity = [](auto const a) { return a; };
}

#endif