#ifndef MIX_DD_MORE_ALGORITHM_HPP
#define MIX_DD_MORE_ALGORITHM_HPP

#include "../utils/more_vector.hpp"

#include <algorithm>
#include <iterator>
#include <type_traits>

namespace mix::utils
{
    namespace impl
    {
        template<class InputIt, class UnaryOperation>
        auto map(InputIt first, InputIt last, std::size_t const count, UnaryOperation op)
        {
            auto result = utils::vector<decltype(op(*first))>(count);
            std::transform(first, last, std::back_inserter(result), op);
            return result;
        }
    }

    /**
        @brief Wrap around std::transform that saves the result into a std::vector.
     */
    template<class InputIt, class UnaryOperation>
    auto map(InputIt first, InputIt last, UnaryOperation op)
    {
        using input_it_category = typename std::iterator_traits<InputIt>::iterator_category;
        auto constexpr hasFastCount = std::is_same_v< input_it_category
                                                    , std::random_access_iterator_tag >;
        if constexpr (hasFastCount)
        {
            return impl::map(first, last, std::distance(first, last), op);
        }
        else
        {
            return impl::map(first, last, 4ul, op);
        }
    }

    /**
     *  @brief Wrap around std::transform that saves the result into a std::vector.
     */
    template<class Range, class UnaryOperation>
    auto map(Range&& range, UnaryOperation op)
    {
        return map(std::begin(range), std::end(range), op);
    }

    /**
     *  @brief Wrap around std::transform that saves the result into a std::vector.
     */
    template<class Range, class UnaryOperation>
    auto map(Range&& range, std::size_t const count, UnaryOperation op)
    {
        return impl::map(std::begin(range), std::end(range), count, op);
    }
}

#endif