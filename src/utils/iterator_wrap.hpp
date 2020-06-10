#ifndef MIX_UTILS_ITERATOR_WRAP_
#define MIX_UTILS_ITERATOR_WRAP_

#include <tuple>
#include <iterator>

namespace mix::utils
{
    namespace aux_impl
    {
        inline auto deref = [](auto&&... xs)
        {
            return std::forward_as_tuple((*xs)...);
        };

        inline auto prefix_inc = [](auto&&... xs)
        {
            (++xs, ...);
        };

        template <std::size_t ... Is, typename Tuple>
        auto any_equal_impl (std::index_sequence<Is...>, Tuple const& t1, Tuple const& t2)
        {
            return ((std::get<Is>(t1) == std::get<Is>(t2)) || ...);
        }

        template<class Tuple>
        auto any_equal (Tuple const& t1, Tuple const& t2)
        {
            return any_equal_impl(std::make_index_sequence<std::tuple_size_v<Tuple>>(), t1, t2);
        }

        template<template<class> class ItType, class ItTuple>
        class impl_base
        {
        public:
            using iterator       = ItType<ItTuple>;
            using const_iterator = ItType<ItTuple>;

            impl_base(ItTuple begins, ItTuple ends);

            auto begin ()       -> iterator;
            auto end   ()       -> iterator;
            auto begin () const -> const_iterator;
            auto end   () const -> const_iterator;
        
        private:
            ItTuple begins_;
            ItTuple ends_;
        };

        template<class ItTuple>
        class iterator_base
        {
        public:
            iterator_base(ItTuple const& its, ItTuple const& ends);

            auto operator== (iterator_base const& o) const -> bool;
            auto operator!= (iterator_base const& o) const -> bool;
            auto operator*  () const -> decltype(std::apply(aux_impl::deref, std::declval<ItTuple&>()));

        protected:
            ItTuple        its_;
            ItTuple const& begins_;
            ItTuple const& ends_;
        };

        template<class ItTuple>
        class zip_iterator : public iterator_base<ItTuple>
        {
        public:
            zip_iterator(ItTuple const& its, ItTuple const& ends);
            auto operator++ () -> zip_iterator&;
        };

        template<class ItTuple>
        class product_iterator : public iterator_base<ItTuple>
        {
        public:
            product_iterator(ItTuple const& its, ItTuple const& ends);
            auto operator++ () -> product_iterator&;
        };

        template<template<class> class ItType, class ItTuple>
        impl_base<ItType, ItTuple>::impl_base(ItTuple begins, ItTuple ends) :
            begins_ {std::move(begins)},
            ends_   {std::move(ends)}
        {
        }

        template<template<class> class ItType, class ItTuple>
        auto impl_base<ItType, ItTuple>::begin
            () -> iterator
        {
            return iterator {begins_, ends_};
        }

        template<template<class> class ItType, class ItTuple>
        auto impl_base<ItType, ItTuple>::end
            () -> iterator
        {
            return iterator {ends_, ends_};
        }

        template<template<class> class ItType, class ItTuple>
        auto impl_base<ItType, ItTuple>::begin
            () const -> const_iterator
        {
            return const_iterator {begins_, ends_};
        }

        template<template<class> class ItType, class ItTuple>
        auto impl_base<ItType, ItTuple>::end
            () const -> const_iterator
        {
            return const_iterator {ends_, ends_};
        }

        template<class ItTuple>
        iterator_base<ItTuple>::iterator_base(ItTuple const& its, ItTuple const& ends) :
            its_    {its},
            begins_ {its},
            ends_   {ends}
        {
        }

        template<class ItTuple>
        auto iterator_base<ItTuple>::operator==
            (const iterator_base& o) const -> bool
        {
            return its_ == o.its_;
        }

        template<class ItTuple>
        auto iterator_base<ItTuple>::operator!=
            (const iterator_base& o) const -> bool
        {
            return ! (*this == o);
        }

        template<class ItTuple>
        auto iterator_base<ItTuple>::operator*
            () const -> decltype(std::apply(deref, std::declval<ItTuple&>()))
        {
            return std::apply(aux_impl::deref, its_);
        }

        template<class ItTuple>
        zip_iterator<ItTuple>::zip_iterator
            (ItTuple const& its, ItTuple const& ends) :
            iterator_base<ItTuple> {its, ends}
        {
        }

        template<class ItTuple>
        auto zip_iterator<ItTuple>::operator++
            () -> zip_iterator&
        {
            using base_t = iterator_base<ItTuple>;

            std::apply(prefix_inc, base_t::its_);
            if (any_equal(base_t::its_, base_t::ends_))
            {
                base_t::its_ = base_t::ends_;
            }
            
            return *this;
        }

        template<class ItTuple>
        product_iterator<ItTuple>::product_iterator
            (ItTuple const& its, ItTuple const& ends) :
            iterator_base<ItTuple> {its, ends}
        {
        }

        template<size_t I, size_t N, class ItTuple, class CItTuple>
        auto advance_iterators (ItTuple& its, CItTuple&& begins, CItTuple&& ends) -> std::enable_if_t<I == N - 1, bool>
        {
            std::advance(std::get<I>(its), 1);

            if (std::get<I>(its) == std::get<I>(ends))
            {
                std::get<I>(its) = std::get<I>(begins);
                return false;
            }

            return true;
        }

        template<size_t I, size_t N, class ItTuple>
        auto advance_iterators (ItTuple& its, ItTuple const& begins, ItTuple const& ends) -> std::enable_if_t<I < N - 1, bool>
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
            using base_t = iterator_base<ItTuple>;
            
            if (! advance_iterators<0, std::tuple_size_v<ItTuple>>(base_t::its_, base_t::begins_, base_t::ends_))
            {
                base_t::its_ = base_t::ends_;
            }

            return *this;
        }

        template<class ItTuple, class... Cs>
        auto zip_helper(Cs&&... cs)
        {
            using zip_impl = aux_impl::impl_base<aux_impl::zip_iterator, ItTuple>;
            return zip_impl { std::make_tuple(std::begin(std::forward<Cs>(cs))...) 
                            , std::make_tuple(std::end(std::forward<Cs>(cs))...) };
        }

        template<class ItTuple, class... Cs>
        auto product_helper(Cs&&... cs)
        {
            using product_impl = aux_impl::impl_base<aux_impl::product_iterator, ItTuple>;
            return product_impl { std::make_tuple(std::begin(std::forward<Cs>(cs))...) 
                                , std::make_tuple(std::end(std::forward<Cs>(cs))...) };
        }
    }

    template<class... Cs>
    auto zip(Cs const&... cs)
    {
        using tuple_t = decltype(std::make_tuple(std::begin(cs)...));
        return aux_impl::zip_helper<tuple_t>(cs...);
    }

    template<class... Cs>
    auto zip_mutable(Cs&... cs)
    {
        using tuple_t = decltype(std::make_tuple(std::begin(cs)...));
        return aux_impl::zip_helper<tuple_t>(cs...);
    }

    template<class... Cs>
    auto product(Cs const&... cs)
    {
        using tuple_t = decltype(std::make_tuple(std::begin(cs)...));
        return aux_impl::product_helper<tuple_t>(cs...);
    }

    template<class... Cs>
    auto product_mutable(Cs&... cs)
    {
        using tuple_t = decltype(std::make_tuple(std::begin(cs)...));
        return aux_impl::product_helper<tuple_t>(cs...);
    }
}

#endif