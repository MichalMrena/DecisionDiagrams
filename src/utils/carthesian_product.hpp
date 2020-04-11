#ifndef MIX_UTILS_CARTHESIAN_PRODUCT_
#define MIX_UTILS_CARTHESIAN_PRODUCT_

#include <tuple>
#include <utility>
#include <type_traits>

namespace mix::utils
{
    template<class... Cs>
    class product_iterator;

    /**
     *  Non owning container view that can iterate over carthesian product
     *  of given containers. Container must have const_reference and
     *  const_iterator member types. 
     *  And of course begin() and end() member functions.
     * 
     *  Example:
     *  auto const as = std::vector<int>    {1, 2, 3};
     *  auto const bs = std::vector<char>   {'a', 'b', 'c'};
     *  auto const cs = std::vector<double> {0.1, 3.14};
     *  for (auto&& [e1, e2, e3] : product {as, bs, cs})
     *  {
     *      std::cout << e1 << " " << e2 << " " << e3 << '\n';
     *  }
    */
    template<class... Cs>
    class product
    {
    public:
        using const_iterator  = product_iterator<Cs...>;
        using product_tuple   = std::tuple<typename Cs::const_reference...>;
        using iterators_tuple = std::tuple<typename Cs::const_iterator...>;
        
        friend class product_iterator<Cs...>;

    public:
        product(const Cs&... cs);

        auto begin ()       -> const_iterator;
        auto end   ()       -> const_iterator;
        auto begin () const -> const_iterator;
        auto end   () const -> const_iterator;

    private:
        iterators_tuple begins_;
        iterators_tuple ends_;
    };

    template<class... Cs>
    class product_iterator
    {
    public:
        using product_tuple   = typename product<Cs...>::product_tuple;
        using product_cref    = const product<Cs...>&;
        using iterators_tuple = typename product<Cs...>::iterators_tuple;

        inline static constexpr auto N = sizeof...(Cs);

    public:
        product_iterator(product_cref product, bool isEnd);

        auto operator!= (const product_iterator& o) const -> bool;
        auto operator*  () const -> product_tuple;
        auto operator++ ()       -> product_iterator&;

    private:
        product_cref    product_;
        iterators_tuple iterators_;
        bool            isEnd_;
    };

    template<class... Cs>
    product<Cs...>::product(const Cs&... cs) :
        begins_     { std::make_tuple(std::begin(cs)...) }
      , ends_       { std::make_tuple(std::end(cs)...)   }
    {
    }

    template<class... Cs>
    auto product<Cs...>::product::begin
        () -> const_iterator
    {
        return const_iterator {*this, false};
    }

    template<class... Cs>
    auto product<Cs...>::product::end
        () -> const_iterator
    {
        return const_iterator {*this, true};
    }

    template<class... Cs>
    auto product<Cs...>::product::begin
        () const -> const_iterator
    {
        return const_iterator {*this, false};
    }

    template<class... Cs>
    auto product<Cs...>::product::end
        () const -> const_iterator
    {
        return const_iterator {*this, true};
    }

    namespace aux_impl
    {
        template<size_t I, size_t N, class IterTuple, class CIterTuple>
        auto advance_iterators (IterTuple& its, CIterTuple&& begins, CIterTuple&& ends) -> std::enable_if_t<I == N - 1, bool>
        {
            std::advance(std::get<I>(its), 1);

            if (std::get<I>(its) == std::get<I>(ends))
            {
                std::get<I>(its) = std::get<I>(begins);
                return true;
            }

            return false;
        }

        template<size_t I, size_t N, class IterTuple, class CIterTuple>
        auto advance_iterators (IterTuple& its, CIterTuple&& begins, CIterTuple&& ends) -> std::enable_if_t<I < N - 1, bool>
        {
            std::advance(std::get<I>(its), 1);

            if (std::get<I>(its) == std::get<I>(ends))
            {
                std::get<I>(its) = std::get<I>(begins);
                return advance_iterators<I + 1, N>(its, begins, ends);
            }

            return false;
        }
    }

    template<class... Cs>
    product_iterator<Cs...>::product_iterator(product_cref product, bool isEnd) :
        product_   { product          }
      , iterators_ { product_.begins_ }
      , isEnd_     { isEnd            }
    {
    }

    template<class... Cs>
    auto product_iterator<Cs...>::operator!=
        (const product_iterator& o) const -> bool
    {
        return isEnd_ != o.isEnd_ || iterators_ != o.iterators_;
    }

    template<class... Cs>
    auto product_iterator<Cs...>::operator*
        () const -> product_tuple
    {
        auto deref = [](auto&&... is)
        {
            return std::forward_as_tuple(*is...);
        };

        return std::apply(deref, iterators_);
    }

    template<class... Cs>
    auto product_iterator<Cs...>::operator++
        () -> product_iterator&
    {
        isEnd_ = aux_impl::advance_iterators<0, N>(iterators_, product_.begins_, product_.ends_);

        return *this;
    }
}

#endif