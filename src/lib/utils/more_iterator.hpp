#ifndef MIX_UTILS_MORE_ITERATOR_HPP
#define MIX_UTILS_MORE_ITERATOR_HPP

#include <iterator>

namespace mix::utils
{
    namespace detail
    {
        /**
         *  @brief Range iterator.
         */
        template<class IntType>
        class range_iterator
        {
        public:
            using difference_type   = std::ptrdiff_t;
            using value_type        = IntType;
            using pointer           = IntType*;
            using reference         = IntType&;
            using iterator_category = std::input_iterator_tag;

            range_iterator(IntType curr);

            auto operator== (range_iterator const& o) const -> bool;
            auto operator!= (range_iterator const& o) const -> bool;
            auto operator*  () const                        -> value_type;
            auto operator++ ()                              -> range_iterator&;

        private:
            IntType curr_;
        };

        /**
         *  @brief Iterable range.
         */
        template<class IntType>
        class range_holder
        {
        public:
            using iterator       = range_iterator<IntType>;
            using const_iterator = range_iterator<IntType>;

            range_holder(IntType first, IntType last);

            auto begin  ()       -> iterator;
            auto end    ()       -> iterator;
            auto begin  () const -> const_iterator;
            auto end    () const -> const_iterator;
            auto cbegin () const -> const_iterator;
            auto cend   () const -> const_iterator;

        private:
            IntType first_;
            IntType last_;
        };

        template<class IntType>
        range_holder<IntType>::range_holder
            (IntType const first, IntType const last) :
            first_ {first},
            last_  {last}
        {
        }

        template<class IntType>
        auto range_holder<IntType>::begin
            () -> iterator
        {
            return iterator {first_};
        }

        template<class IntType>
        auto range_holder<IntType>::end
            () -> iterator
        {
            return iterator {last_};
        }

        template<class IntType>
        auto range_holder<IntType>::begin
            () const -> const_iterator
        {
            return const_iterator {first_};
        }

        template<class IntType>
        auto range_holder<IntType>::end
            () const -> const_iterator
        {
            return const_iterator {last_};
        }

        template<class IntType>
        auto range_holder<IntType>::cbegin
            () const -> const_iterator
        {
            return const_iterator {first_};
        }

        template<class IntType>
        auto range_holder<IntType>::cend
            () const -> const_iterator
        {
            return const_iterator {last_};
        }

        template<class IntType>
        range_iterator<IntType>::range_iterator
            (IntType const curr) :
            curr_ {curr}
        {
        }

        template<class IntType>
        auto range_iterator<IntType>::operator==
            (range_iterator const& o) const -> bool
        {
            return curr_ == o.curr_;
        }

        template<class IntType>
        auto range_iterator<IntType>::operator!=
            (range_iterator const& o) const -> bool
        {
            return !(*this == o);
        }

        template<class IntType>
        auto range_iterator<IntType>::operator*
            () const -> value_type
        {
            return curr_;
        }

        template<class IntType>
        auto range_iterator<IntType>::operator++
            ()  -> range_iterator&
        {
            ++curr_;
            return *this;
        }
    }

    /**
     *  @brief creates an iterable object which represents range [ @p first, @p last ).
     */
    template<class IntType>
    auto range(IntType const first, IntType const last)
    {
        return detail::range_holder<IntType>(first, last);
    }
}

#endif