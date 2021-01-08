#ifndef MIX_DD_TEST_TEST_BASE_HPP
#define MIX_DD_TEST_TEST_BASE_HPP

#include "../lib/utils/more_random.hpp"

#include <vector>
#include <limits>
#include <algorithm>

namespace mix::dd::test
{
    enum class order_e
    {
        Default,
        Random
    };

    auto constexpr UIntMax = std::numeric_limits<unsigned int>::max();

    inline auto get_default_order(std::size_t const varCount)
    {
        auto is = std::vector<index_t>(varCount);
        std::iota(std::begin(is), std::end(is), 0);
        return is;
    }

    inline auto get_random_order(std::mt19937& rngOrder, std::size_t const varCount)
    {
        auto is = get_default_order(varCount);
        std::shuffle(std::begin(is), std::end(is), rngOrder);
        return is;
    }

    inline auto get_order(order_e const o, std::mt19937& rngOrder, std::size_t const varCount)
    {
        switch (o)
        {
            case order_e::Default: return get_default_order(varCount);
            case order_e::Random:  return get_random_order(rngOrder, varCount);
            default: throw "not good";
        }
    }
}

#endif