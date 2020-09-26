#ifndef MIX_DD_MORE_ALGORITHM_HPP
#define MIX_DD_MORE_ALGORITHM_HPP

#include "../utils/more_vector.hpp"

#include <algorithm>
#include <iterator>

namespace mix::utils
{
    /**
     *  @brief Wrap around std::transform that saves the result into a std::vector.
     */
    template<class InputIt, class UnaryOperation>
    auto map(InputIt first, InputIt last, UnaryOperation op)
    {
        auto result = utils::vector<decltype(op(*first))>(std::distance(first, last));
        std::transform(first, last, std::back_inserter(result), op);
        return result;
    }

    /**
     *  @brief Wrap around std::transform that saves the result into a std::vector.
     */
    template<class Range, class UnaryOperation>
    auto map(Range&& range, UnaryOperation op)
    {
        return map(std::begin(range), std::end(range), op);
    }
}

#endif