#ifndef MIX_UTILS_MORE_ITERATOR_HPP
#define MIX_UTILS_MORE_ITERATOR_HPP

#include <tuple>
#include <iterator>
#include <type_traits>

namespace mix::utils
{
    namespace mi_impl
    {
        inline auto deref = [](auto&&... xs)
        {
            return std::tuple<decltype(*xs)...>((*xs)...);
        };

        inline auto prefix_inc = [](auto&&... xs)
        {
            (++xs, ...);
        };

        inline auto get_begin = [](auto&&... cs)
        {
            return std::make_tuple(std::begin(cs)...);
        };

        inline auto get_end = [](auto&&... cs)
        {
            return std::make_tuple(std::end(cs)...);
        };

        inline auto get_cbegin = [](auto&&... cs)
        {
            return std::make_tuple(std::cbegin(cs)...);
        };

        inline auto get_cend = [](auto&&... cs)
        {
            return std::make_tuple(std::cend(cs)...);
        };

        template <std::size_t... Is, typename Tuple>
        auto any_equal_impl (std::index_sequence<Is...>, Tuple const& t1, Tuple const& t2)
        {
            return ((std::get<Is>(t1) == std::get<Is>(t2)) || ...);
        }

        template<class Tuple>
        auto any_equal (Tuple const& t1, Tuple const& t2)
        {
            return any_equal_impl(std::make_index_sequence<std::tuple_size_v<Tuple>>(), t1, t2);
        }

        /**
            Container holder.
         */
        template<template<class> class ItType, class ContainerTuple>
        class container_holder
        {
        public:
            using ItTuple        = decltype(std::apply(get_begin,  std::declval<ContainerTuple&>()));
            using CItTuple       = decltype(std::apply(get_cbegin, std::declval<ContainerTuple&>()));
            using iterator       = ItType<ItTuple>;
            using const_iterator = ItType<CItTuple>;

            template<class CTupleHelper>
            container_holder(CTupleHelper&& cs);

            auto begin  ()       -> iterator;
            auto end    ()       -> iterator;
            auto begin  () const -> const_iterator;
            auto end    () const -> const_iterator;
            auto cbegin () const -> const_iterator;
            auto cend   () const -> const_iterator;

        private:
            ContainerTuple cs_;
        };

        /**
            Iterator holder.
         */
        template<class ItTuple>
        class iterator_holder
        {
        public:
            using value_type        = decltype(std::apply(deref, std::declval<ItTuple&>()));
            using pointer           = value_type*;
            using reference         = value_type&;
            using difference_type   = std::ptrdiff_t;
            using iterator_category = std::input_iterator_tag;

            template<class ItTupleRef>
            iterator_holder(ItTupleRef&& its, ItTupleRef&& ends);

            auto operator== (iterator_holder const& o) const -> bool;
            auto operator!= (iterator_holder const& o) const -> bool;
            auto operator*  () const -> value_type;

        protected:
            ItTuple its_;
            ItTuple begins_;
            ItTuple ends_;
        };

        /**
            Zip iterator.
         */
        template<class ItTuple>
        class zip_iterator : public iterator_holder<ItTuple>
        {
        public:
            template<class ItTupleRef>
            zip_iterator(ItTupleRef&& its, ItTupleRef&& ends);
            auto operator++ () -> zip_iterator&;
        };

        /**
            Cartesian product iterator.
         */
        template<class ItTuple>
        class product_iterator : public iterator_holder<ItTuple>
        {
        public:
            template<class ItTupleRef>
            product_iterator(ItTupleRef&& its, ItTupleRef&& ends);
            auto operator++ () -> product_iterator&;
        };

        /**
            Range iterator.
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

            range_iterator(IntType const curr);

            auto operator== (range_iterator const& o) const -> bool;
            auto operator!= (range_iterator const& o) const -> bool;
            auto operator*  () const                        -> IntType;
            auto operator++ ()                              -> range_iterator&;

        private:
            IntType curr_;
        };

        /**
            Range holder.
         */
        template<class IntType>
        class range_holder
        {
        public:
            using iterator       = range_iterator<IntType>;
            using const_iterator = range_iterator<IntType>;

            range_holder(IntType const first, IntType const last);

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

        template<template<class> class ItType, class ContainerTuple>
        template<class CTupleHelper>
        container_holder<ItType, ContainerTuple>::container_holder(CTupleHelper&& cs) :
            cs_ {std::forward<CTupleHelper>(cs)}
        {
        }

        template<template<class> class ItType, class ContainerTuple>
        auto container_holder<ItType, ContainerTuple>::begin
            () -> iterator
        {
            return iterator {std::apply(get_begin, cs_), std::apply(get_end, cs_)};
        }

        template<template<class> class ItType, class ContainerTuple>
        auto container_holder<ItType, ContainerTuple>::end
            () -> iterator
        {
            return iterator {std::apply(get_end, cs_), std::apply(get_end, cs_)};
        }

        template<template<class> class ItType, class ContainerTuple>
        auto container_holder<ItType, ContainerTuple>::begin
            () const -> const_iterator
        {
            return const_iterator {std::apply(get_cbegin, cs_), std::apply(get_cend, cs_)};
        }

        template<template<class> class ItType, class ContainerTuple>
        auto container_holder<ItType, ContainerTuple>::end
            () const -> const_iterator
        {
            return const_iterator {std::apply(get_cend, cs_), std::apply(get_cend, cs_)};
        }

        template<template<class> class ItType, class ContainerTuple>
        auto container_holder<ItType, ContainerTuple>::cbegin
            () const -> const_iterator
        {
            return const_iterator {std::apply(get_cbegin, cs_), std::apply(get_cend, cs_)};
        }

        template<template<class> class ItType, class ContainerTuple>
        auto container_holder<ItType, ContainerTuple>::cend
            () const -> const_iterator
        {
            return const_iterator {std::apply(get_cend, cs_), std::apply(get_cend, cs_)};
        }

        template<class ItTuple>
        template<class ItTupleRef>
        iterator_holder<ItTuple>::iterator_holder
            (ItTupleRef&& its, ItTupleRef&& ends) :
            its_    {std::forward<ItTupleRef>(its)},
            begins_ {its_},
            ends_   {std::forward<ItTupleRef>(ends)}
        {
        }

        template<class ItTuple>
        auto iterator_holder<ItTuple>::operator==
            (iterator_holder const& o) const -> bool
        {
            return its_ == o.its_;
        }

        template<class ItTuple>
        auto iterator_holder<ItTuple>::operator!=
            (iterator_holder const& o) const -> bool
        {
            return ! (*this == o);
        }

        template<class ItTuple>
        auto iterator_holder<ItTuple>::operator*
            () const -> value_type
        {
            return std::apply(mi_impl::deref, its_);
        }

        template<class ItTuple>
        template<class ItTupleRef>
        zip_iterator<ItTuple>::zip_iterator
            (ItTupleRef&& its, ItTupleRef&& ends) :
            iterator_holder<ItTuple> {its, ends}
        {
        }

        template<class ItTuple>
        auto zip_iterator<ItTuple>::operator++
            () -> zip_iterator&
        {
            using base_t = iterator_holder<ItTuple>;

            std::apply(prefix_inc, base_t::its_);
            if (any_equal(base_t::its_, base_t::ends_))
            {
                base_t::its_ = base_t::ends_;
            }

            return *this;
        }

        template<class ItTuple>
        template<class ItTupleRef>
        product_iterator<ItTuple>::product_iterator
            (ItTupleRef&& its, ItTupleRef&& ends) :
            iterator_holder<ItTuple> {its, ends}
        {
        }

        template<std::size_t I, std::size_t N, class ItTuple, class CItTuple>
        auto advance_iterators
            (ItTuple& its, CItTuple&& begins, CItTuple&& ends) -> std::enable_if_t<I == N - 1, bool>
        {
            std::advance(std::get<I>(its), 1);

            if (std::get<I>(its) == std::get<I>(ends))
            {
                std::get<I>(its) = std::get<I>(begins);
                return false;
            }

            return true;
        }

        template<std::size_t I, std::size_t N, class ItTuple>
        auto advance_iterators
            (ItTuple& its, ItTuple const& begins, ItTuple const& ends) -> std::enable_if_t<I < N - 1, bool>
        {
            std::advance(std::get<I>(its), 1);

            if (std::get<I>(its) == std::get<I>(ends))
            {
                std::get<I>(its) = std::get<I>(begins);
                return advance_iterators<I + 1, N>(its, begins, ends);
            }

            return true;
        }

        template<class ItTuple>
        auto product_iterator<ItTuple>::operator++
            () -> product_iterator&
        {
            using base_t = iterator_holder<ItTuple>;

            if (!advance_iterators<0, std::tuple_size_v<ItTuple>>(base_t::its_, base_t::begins_, base_t::ends_))
            {
                base_t::its_ = base_t::ends_;
            }

            return *this;
        }

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
            () const -> IntType
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

        template<class C>
        auto to_tuple_element(C&& c) -> decltype(auto)
        {
            if constexpr (std::is_reference_v<C>)
            {
                return c;
            }
            else
            {
                return C(std::move(c));
            }
        }

        template<std::size_t I, class T>
        auto elem_id (T const& t)
        {
            return t;
        }

        template<class T, std::size_t... Is>
        auto repeat_impl (T const& t, std::index_sequence<Is...>)
        {
            return std::tuple {elem_id<Is>(t)...};
        }
    }

    /**
        @brief Makes tuple with arity N of copies of @p t .
     */
    template<std::size_t N, class T>
    auto repeat (T const& t)
    {
        return mi_impl::repeat_impl(t, std::make_index_sequence<N>());
    }

    /**
        @brief Makes iterable object from container references @p cs .
        Iterator dereferences to tuple of references of elements of @p cs .
        If a @p cs is rvalue reference, new container is move constructed
        in the object. Otherwise, only a reference to a container is stored.
     */
    template<class... Cs>
    auto zip(Cs&&... cs)
    {
        using tuple_t  = std::tuple<decltype(mi_impl::to_tuple_element(std::forward<Cs>(cs)))...>;
        using holder_t = mi_impl::container_holder<mi_impl::zip_iterator, tuple_t>;
        return holder_t {tuple_t {mi_impl::to_tuple_element(std::forward<Cs>(cs))...}};
    }

    /**
        @brief Makes iterable object from references to containers @p cs .
        Iterator dereferences to tuple of references of elements of @p cs .
        If a @p cs is rvalue reference, new container is move constructed
        in the object. Otherwise, only a reference to a container is stored.
     */
    template<class... Cs>
    auto product(Cs&&... cs)
    {
        using tuple_t  = std::tuple<decltype(mi_impl::to_tuple_element(std::forward<Cs>(cs)))...>;
        using holder_t = mi_impl::container_holder<mi_impl::product_iterator, tuple_t>;
        return holder_t {tuple_t {mi_impl::to_tuple_element(std::forward<Cs>(cs))...}};
    }

    /**
        @brief creates an iterable object which represents range [first, last).
     */
    template<class FirstType, class SecondType>
    auto range(FirstType const first, SecondType const last)
    {
        using ctype = std::common_type_t<FirstType, SecondType>;
        return mi_impl::range_holder<ctype> {first, last};
    }
}

#endif