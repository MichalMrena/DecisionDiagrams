#ifndef MIX_DD_MORE_ALGORITHM_HPP
#define MIX_DD_MORE_ALGORITHM_HPP

#include "more_vector.hpp"
#include "more_type_traits.hpp"

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>
#include <limits>

namespace mix::utils
{
    /**
        @brief Wrap around std::transform that saves the result into std::vector.
        @param count size of the range so that resulting vector can be allocated at once.
     */
    template<class InputIt, class UnaryOperation>
    auto fmap(InputIt first, InputIt last, std::size_t const count, UnaryOperation op)
    {
        using T = decltype(op(*first));
        auto result = utils::vector<T>(count);
        std::transform(first, last, std::back_inserter(result), op);
        return result;
    }

    /**
        @brief Wrap around std::transform that saves the result into std::vector.
     */
    template<class InputIt, class UnaryOperation>
    auto fmap(InputIt first, InputIt last, UnaryOperation op)
    {
        if constexpr (is_random_access_v<InputIt>)
        {
            auto const count = static_cast<std::size_t>(std::distance(first, last));
            return fmap(first, last, count, op);
        }
        else
        {
            return fmap(first, last, 4ul, op);
        }
    }

    /**
        @brief Wrap around std::transform that saves the result into std::vector.
     */
    template<class Range, class UnaryOperation>
    auto fmap(Range&& range, UnaryOperation op)
    {
        return fmap(std::begin(range), std::end(range), op);
    }

    /**
        @brief Wrap around std::transform that saves the result into a std::vector.
        @param count size of the @p range so that resulting vector can be allocated at once.
     */
    template<class Range, class UnaryOperation>
    auto fmap(Range&& range, std::size_t const count, UnaryOperation op)
    {
        return fmap(std::begin(range), std::end(range), count, op);
    }

    /**
        @brief Same as fmap but also passes an index as an argument to @p op .
     */
    template<class Range, class UnaryOperation>
    auto fmap_i(Range&& range, UnaryOperation op)
    {
        return fmap(range, [op, i = 0u](auto&& x) mutable
        {
            return op(i++, x);
        });
    }

    /**
        @brief like fmap but uses predicate to determine whether element should be mapped.
        @param count size of the range so that resulting vector can be allocated at once.
     */
    template<class InputIt, class Predicate, class UnaryOperation>
    auto filter_fmap(InputIt first, InputIt last, std::size_t const count, Predicate p, UnaryOperation op)
    {
        using T = decltype(op(*first));
        auto result = utils::vector<T>(count);
        while (first != last)
        {
            if (p(*first))
            {
                result.push_back(op(*first));
            }
            ++first;
        }
        result.shrink_to_fit();
        return result;
    }

    /**
        @brief like fmap but uses predicate to determine whether element should be mapped.
     */
    template<class InputIt, class Predicate, class UnaryOperation>
    auto filter_fmap(InputIt first, InputIt last, Predicate p, UnaryOperation op)
    {
        if constexpr (is_random_access_v<InputIt>)
        {
            auto const size = static_cast<std::size_t>(std::distance(first, last));
            return filter_fmap(first, last, size, p, op);
        }
        else
        {
            return filter_fmap(first, last, 4ul, p, op);
        }
    }

    /**
        @brief like fmap but uses predicate to determine whether element should be mapped.
     */
    template<class Range, class Predicate, class UnaryOperation>
    auto filter_fmap(Range&& range, Predicate p, UnaryOperation op)
    {
        return filter_fmap(std::begin(range), std::end(range), p, op);
    }

    /**
        @brief Wrap around std::transform that saves the result into std::array.
     */
    template<std::size_t N, class InputIt, class UnaryOperation>
    auto fmap_to_array(InputIt first, InputIt last, UnaryOperation op)
    {
        using T  = decltype(op(*std::declval<InputIt>()));
        auto ret = std::array<T, N>{};
        std::transform(first, last, std::begin(ret), op);
        return ret;
    }

    /**
        @brief Wrap around std::transform that saves the result into a std::array.
     */
    template<class T, std::size_t N, class UnaryOperation>
    auto fmap_to_array(std::array<T, N> const& as, UnaryOperation op)
    {
        return fmap_to_array<N>(std::begin(as), std::end(as), op);
    }

    /**
        @brief Wrap around std::transform that saves the result into a std::array.
     */
    template<std::size_t N, class Range, class UnaryOperation>
    auto fmap_to_array(Range&& range, UnaryOperation op)
    {
        return fmap_to_array<N>(std::begin(range), std::end(range), op);
    }

    /**
        @brief Fills an array with values generated from function @p f .
        @param f function that has one parameter of an integral type (index).
        @param n (< N) number of values to generate. Rest is value initialized.
        @return std::array<decltype(f(0u)), N> of elements generated by @p f .
     */
    template<std::size_t N, class Generator>
    auto constexpr fill_array_n(std::size_t const n, Generator&& f)
    {
        using T          = decltype(f(0u));
        auto ret         = std::array<T, N> {};
        auto const begin = std::begin(ret);
        auto const end   = std::next(begin, static_cast<std::ptrdiff_t>(n));
        std::generate(begin, end, [i = 0u, &f]() mutable
        {
            return f(i++);
        });
        return ret;
    }

    /**
        @brief Fills an array with values generated from function @p f .
        @param f function that has one parameter of an integral type (index).
        @return std::array<decltype(f(0u)), N> of elements generated by @p f .
     */
    template<std::size_t N, class Generator>
    auto constexpr fill_array(Generator&& f)
    {
        return fill_array_n<N>(N, f);
    }

    /**
        @brief Fills a vector with values generated from a generating function @p f .
        @param n size of the vector.
        @param f function that has one parameter of an integral type (index).
        @return std::vector<decltype(f(0u))> of elements generated by @p f .
     */
    template<class Generator>
    auto fill_vector(std::size_t const n, Generator&& f)
    {
        using T  = decltype(f(0u));
        auto ret = utils::vector<T>(n);
        for (auto i = 0u; i < n; ++i)
        {
            ret.emplace_back(f(i));
        }
        return ret;
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