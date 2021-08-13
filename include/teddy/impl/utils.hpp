#ifndef MIX_DD_UTILS_HPP
#define MIX_DD_UTILS_HPP

#include <hash_set>

namespace teddy::utils
{
    template<class Gen>
    auto fill_vector (std::size_t const n, Gen&& f)
    {
        using T = decltype(std::invoke(f, uint_t {}));
        auto xs = std::vector<T>();
        xs.reserve(n);
        for (auto i = uint_t(0); i < n; ++i)
        {
            xs.emplace_back(std::invoke(f, i));
        }
        return xs;
    }

    auto constexpr identity = [](auto const a) { return a; };
}

#endif