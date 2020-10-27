#ifndef MIX_DD_MORE_ALGORITHM_HPP
#define MIX_DD_MORE_ALGORITHM_HPP

#include "../utils/more_vector.hpp"

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>
#include <limits>

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
        @brief Wrap around std::transform that saves the result into a std::vector.
     */
    template<class Range, class UnaryOperation>
    auto map(Range&& range, UnaryOperation op)
    {
        return map(std::begin(range), std::end(range), op);
    }

    /**
        @brief Wrap around std::transform that saves the result into a std::vector.
     */
    template<class Range, class UnaryOperation>
    auto map(Range&& range, std::size_t const count, UnaryOperation op)
    {
        return impl::map(std::begin(range), std::end(range), count, op); // TODO rename to map_to_vector
    }

    /**
        @brief Wrap around std::transform that saves the result into a std::array.
     */
    template<std::size_t N, class InputIt, class UnaryOperation>
    auto map_to_array(InputIt first, InputIt last, UnaryOperation op)
    {
        using newt = decltype(op(*std::declval<InputIt>()));
        auto ret = std::array<newt, N>{};
        std::transform(first, last, std::begin(ret), op);
        return ret;
    }

    /**
        @brief Wrap around std::transform that saves the result into a std::array.
     */
    template<class T, std::size_t N, class UnaryOperation>
    auto map_to_array(std::array<T, N> const& as, UnaryOperation op)
    {
        return map_to_array<N>(std::begin(as), std::end(as), op);
    }

    /**
        @brief Wrap around std::transform that saves the result into a std::array.
     */
    template<std::size_t N, class Range, class UnaryOperation>
    auto map_to_array(Range&& range, UnaryOperation op)
    {
        return map_to_array<N>(std::begin(range), std::end(range), op);
    }

    /**
        @brief Finds index of @p t in given range. If there is no such elemnt returns max std::size_t.
     */
    template<class InputIt, class T>
    auto index_of (InputIt first, InputIt last, T const& t) -> std::size_t
    {
        auto constexpr npos = std::numeric_limits<std::size_t>::max();
        auto pos = std::find(first, last, t);
        return pos != last ? static_cast<std::size_t>(std::distance(first, pos))
                           : npos;
    }
}

#endif