#ifndef _MIX_UTILS_HASH_
#define _MIX_UTILS_HASH_

#include <functional>
#include <cmath>
#include <utility>
#include <tuple>

namespace mix::utils
{
    /**
     *  Computes a hash value for std::tuple, std::pair, std::array
     *  using std::hash and boost::hash_combine formula.
    */
    inline auto tuple_hash = [](auto&& tuple)
    {
        auto seed = 0ull;

        auto acc = [&seed](auto hash)
        {
            seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        auto app = [&acc](auto&&... e) 
        { 
            (acc(std::hash<std::decay_t<decltype(e)>> {} (std::forward<decltype(e)>(e))), ...); 
        };
        
        std::apply(app, std::forward<decltype(tuple)>(tuple));
        
        return seed;
    };

    /**
     *  Helper wrapper so that the above lambda can be easily used as template parameter.
     * 
     *  Example:
     *  using key_t = const std::tuple<const int, const int, const char>;
     *  using val_t = double;
     *  using map_t = std::unordered_map<key_t, val_t, tuple_hash_t<key_t>>; 
     *  auto myMap  = map_t {};
     *  myMap.emplace(std::make_tuple(1, 2, 'a'), 3.14); 
     *  auto pi     = myMap.at(std::make_tuple(1, 2, 'a'));
    */
    template<class Tuple>
    struct tuple_hash_t
    {
        auto operator() (const Tuple& t) const { return tuple_hash(t); }
    };
}

#endif