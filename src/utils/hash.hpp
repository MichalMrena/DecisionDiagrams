#ifndef _MIX_UTILS_HASH_
#define _MIX_UTILS_HASH_

#include <functional>
#include <cmath>
#include <utility>
#include <tuple>

namespace mix::utils
{
    struct hash_accumulator
    {
        size_t seed {0};

        auto operator() (const size_t hash) -> void
        {
            seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };
    };

    template<size_t I, class... Ts>
    auto hash_each_in_tuple 
        (const std::tuple<Ts...>& tuple, hash_accumulator& acc) -> std::enable_if_t<0 != I> 
    {
        std::hash<std::tuple_element_t<I, std::tuple<Ts...>>> hash;
        acc(hash(std::get<I>(tuple)));
        hash_each_in_tuple<I - 1, Ts...>(tuple, acc);
    }

    template<size_t I, class... Ts>
    auto hash_each_in_tuple
        (const std::tuple<Ts...>& tuple, hash_accumulator& acc) -> std::enable_if_t<0 == I>
    {
        std::hash<std::tuple_element_t<I, std::tuple<Ts...>>> hash;
        acc(hash(std::get<0>(tuple)));
    }

    template<class... Ts>
    struct tuple_hash
    {
        auto operator() (const std::tuple<Ts...>& key) const -> size_t
        {
            hash_accumulator acc;
            hash_each_in_tuple< std::tuple_size_v<std::tuple<Ts...>> - 1 >(key, acc);
            return acc.seed;
        }
    };

    template<class T1, class T2>
    struct pair_hash
    {
        auto operator() (const std::pair<T1, T2>& key) const -> size_t
        {
            hash_accumulator acc;
            acc(std::hash<T1> {} (key.first));
            acc(std::hash<T2> {} (key.second));
            return acc.seed;
        }
    };
}

#endif