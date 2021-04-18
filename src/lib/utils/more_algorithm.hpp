#ifndef MIX_DD_MORE_ALGORITHM_HPP
#define MIX_DD_MORE_ALGORITHM_HPP

#include "more_vector.hpp"
#include "more_type_traits.hpp"
#include <algorithm>
#include <functional>
#include <iterator>
#include <utility>

namespace teddy::utils
{
    /**
     *  @brief Wrapper for @c std::transform that saves the result into a @c std::vector .
     *  @param count size of the range so that resulting vector can be allocated at once.
     */
    template<class InputIt, class UnaryOperation>
    auto fmap(InputIt first, InputIt last, std::size_t const count, UnaryOperation op)
    {
        using T     = decltype(std::invoke(op, *first));
        auto result = utils::vector<T>(count);
        std::transform(first, last, std::back_inserter(result), op);
        return result;
    }

    /**
     *  @brief Wrapper for @c std::transform that saves the result into a @c std::vector .
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
     *  @brief Wrapper for @c std::transform that saves the result into a @c std::vector .
     */
    template<class Range, class UnaryOperation>
    auto fmap(Range&& range, UnaryOperation op)
    {
        return fmap(std::begin(range), std::end(range), op);
    }

    /**
     *  @brief Wrapper for @c std::transform that saves the result into a @c std::vector .
     *  @param count size of the @p range so that resulting vector can be allocated at once.
     */
    template<class Range, class UnaryOperation>
    auto fmap(Range&& range, std::size_t const count, UnaryOperation op)
    {
        return fmap(std::begin(range), std::end(range), count, op);
    }

    /**
     *  @brief Same as fmap but also passes an index as an argument to @p op .
     */
    template<class Range, class UnaryOperation>
    auto fmap_i(Range&& range, UnaryOperation op)
    {
        return fmap(range, [op, i = 0u](auto&& x) mutable
        {
            return std::invoke(op, i++, std::forward<decltype(x)>(x));
        });
    }

    /**
     *  @brief like fmap but uses predicate to determine whether element should be mapped.
     *  @param count size of the range so that resulting vector can be allocated at once.
     */
    template<class InputIt, class Predicate, class UnaryOperation>
    auto filter_fmap(InputIt first, InputIt last, std::size_t const count, Predicate p, UnaryOperation op)
    {
        using T = decltype(std::invoke(op, *first));
        auto result = utils::vector<T>(count);
        while (first != last)
        {
            if (std::invoke(p, *first))
            {
                result.push_back(std::invoke(op, *first));
            }
            ++first;
        }
        result.shrink_to_fit();
        return result;
    }

    /**
     *  @brief like fmap but uses predicate to determine whether element should be mapped.
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
     *  @brief like fmap but uses predicate to determine whether element should be mapped.
     */
    template<class Range, class Predicate, class UnaryOperation>
    auto filter_fmap(Range&& range, Predicate p, UnaryOperation op)
    {
        return filter_fmap(std::begin(range), std::end(range), p, op);
    }

    /**
     *  @brief Wrapper for @c std::transform that saves the result into a @c std::array .
     */
    template<std::size_t N, class InputIt, class UnaryOperation>
    auto fmap_to_array(InputIt first, InputIt last, UnaryOperation op)
    {
        using T  = decltype(std::invoke(op, *std::declval<InputIt>()));
        auto ret = std::array<T, N>{};
        std::transform(first, last, std::begin(ret), op);
        return ret;
    }

    /**
     *  @brief Wrapper for @c std::transform that saves the result into a @c std::array .
     */
    template<class T, std::size_t N, class UnaryOperation>
    auto fmap_to_array(std::array<T, N> const& as, UnaryOperation op)
    {
        return fmap_to_array<N>(std::begin(as), std::end(as), op);
    }

    /**
     *  @brief Wrapper for @c std::transform that saves the result into a @c std::array .
     */
    template<std::size_t N, class Range, class UnaryOperation>
    auto fmap_to_array(Range&& range, UnaryOperation op)
    {
        return fmap_to_array<N>(std::begin(range), std::end(range), op);
    }

    /**
     *  @brief Fills an array with values generated from function @p f .
     *  @param f function that has one parameter of an integral type (index).
     *  @param n (< N) number of values to generate. Rest is value initialized.
     *  @return @c std::array<decltype(f(0u)),N> of elements generated by @p f .
     */
    template<std::size_t N, class Generator>
    auto constexpr fill_array_n(std::size_t const n, Generator&& f)
    {
        using T     = decltype(std::invoke(f, 0u));
        auto ret    = std::array<T, N> {};
        for (auto i = 0u; i < n; ++i)
        {
            ret[i]  = std::invoke(f, i);
        }
        return ret;
    }

    /**
     *  @brief Fills an array with values generated from function @p f .
     *  @param f function that has one parameter of an integral type (index).
     *  @return @c std::array<decltype(f(0u)),N> of elements generated by @p f .
     */
    template<std::size_t N, class Generator>
    auto constexpr fill_array(Generator&& f)
    {
        return fill_array_n<N>(N, f);
    }

    /**
     *  @brief Fills a vector with values generated from a generating function @p f .
     *  @param n size of the vector.
     *  @param f function that has one parameter of an integral type (index).
     *  @return @c std::vector<decltype(f(0u))> of elements generated by @p f .
     */
    template<class Generator>
    auto fill_vector(std::size_t const n, Generator&& f)
    {
        using T  = decltype(std::invoke(f, 0u));
        auto ret = utils::vector<T>(n);
        for (auto i = 0u; i < n; ++i)
        {
            ret.emplace_back(std::invoke(f, i));
        }
        return ret;
    }

    /**
     *  @brief Finds index of @p t in [ @p first, @p last ).
     *  If there is no such elemnt returns maximum of @c std::size_t.
     */
    template<class InputIt, class T>
    auto index_of (InputIt first, InputIt last, T const& t) -> std::size_t
    {
        auto constexpr npos = static_cast<std::size_t>(-1);
        auto pos = std::find(first, last, t);
        return pos != last ? static_cast<std::size_t>(std::distance(first, pos))
                           : npos;
    }

    /**
     *  @brief Checks whether all elements in [ @p first, @p last ) are the same.
     */
    template<class InputIt>
    auto all_same (InputIt first, InputIt last) -> bool
    {
        return last == std::adjacent_find(first, last, std::not_equal_to<>());
    }

    /**
     *  @brief Checks whether all elements in the @p range are the same.
     */
    template<class Range>
    auto all_same (Range&& range) -> bool
    {
        return all_same(std::begin(range), std::end(range));
    }

    /**
     *  @brief Wrapper for @c std::for_each.
     */
    template<class Range, class UnaryOperation>
    auto constexpr for_all (Range&& range, UnaryOperation op)
    {
        std::for_each(std::begin(range), std::end(range), op);
    }
}

#endif