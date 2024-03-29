#ifndef LIBTEDDY_TSL_ITERATORS_HPP
#define LIBTEDDY_TSL_ITERATORS_HPP

#include <libteddy/details/types.hpp>

#include <vector>

namespace teddy::tsl
{
/**
 *  \brief Sentinel for domain iterator.
 */
struct domain_iterator_sentinel
{
};

/**
 *  \brief Iterator for domain of a function.
 */
class domain_iterator
{
public:
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::vector<int32>;
    using pointer           = value_type*;
    using reference         = value_type&;
    using iterator_category = std::input_iterator_tag;

public:
    /**
     *  \brief Initializes this as end iterator.
     */
    domain_iterator();

    /**
     *  \brief Initializes using implicit order.
     *  Uses implicit order where x0 is the
     *  least significant (changes most often).
     *  \p domains of individual variables
     */
    domain_iterator(std::vector<int32> domains);

    /**
     *  \brief Initializes using explicitly provided order.
     *
     *  Uses order of variables defined in \p order . Variable with
     *  index \c order[0] changes most often, then variable with
     *  index \c order[1] and so on...
     *  \p domains of individual variables
     *  \p order   order in which variables are incremented
     */
    domain_iterator(std::vector<int32> domains, std::vector<int32> order);

    /**
     *  \brief Initializes using explicitly provided order and fixed values.
     *
     *  Uses order of variables defined in \p order . Variable with
     *  index \c order[0] changes most often, then variable with
     *  index \c order[1] and so on... while skipping variables
     *  defined as fixed by \p fixed .
     *  \p domains of individual variables
     *  \p order   order in which variables are incremented
     *  \p fixed   defines variables with fixed value
     */
    domain_iterator(
        std::vector<int32> domains,
        std::vector<int32> order,
        std::vector<std::pair<int32, int32>> fixed
    );

    auto operator* () const -> std::vector<int32> const&;

    auto operator++ () -> domain_iterator&;

    auto operator++ (int) -> domain_iterator;

    auto operator== (domain_iterator const& rhs) const -> bool;

    auto operator!= (domain_iterator const& rhs) const -> bool;

    auto operator== (domain_iterator_sentinel) const -> bool;

    auto operator!= (domain_iterator_sentinel) const -> bool;

protected:
    std::vector<int32> domains_;
    std::vector<int32> indices_;
    std::vector<int32> varVals_;
};

/**
 *  \brief Sentinel for evaluating iterator.
 */
struct evaluating_iterator_sentinel
{
};

/**
 *  \brief Iterator that evaluates an expression over a domain.
 */
template<class Expression>
class evaluating_iterator
{
public:
    using difference_type   = std::ptrdiff_t;
    using value_type        = int32;
    using pointer           = value_type*;
    using reference         = value_type&;
    using iterator_category = std::input_iterator_tag;

public:
    evaluating_iterator();

    evaluating_iterator(domain_iterator iterator, Expression const& expr);

    auto operator* () const -> int32;

    auto operator++ () -> evaluating_iterator&;

    auto operator++ (int) -> evaluating_iterator;

    auto operator== (evaluating_iterator_sentinel const s) const -> bool;

    auto operator!= (evaluating_iterator_sentinel const s) const -> bool;

    auto get_var_vals () const -> std::vector<int32> const&;

private:
    domain_iterator domainIterator_;
    Expression const* expr_;
};

template<class Expression>
auto operator== (
    evaluating_iterator_sentinel s,
    evaluating_iterator<Expression> const& it
) -> bool
{
    return it == s;
}

template<class Expression>
auto operator!= (
    evaluating_iterator_sentinel s,
    evaluating_iterator<Expression> const& it
) -> bool
{
    return it != s;
}

/**
 *  \brief Output iterator that feeds outputed values into a function.
 */
template<class OutputFunction>
class forwarding_iterator
{
public:
    using difference_type   = std::ptrdiff_t;
    using value_type        = forwarding_iterator&;
    using pointer           = value_type;
    using reference         = value_type;
    using iterator_category = std::output_iterator_tag;

public:
    forwarding_iterator()
    {
    }

    forwarding_iterator(OutputFunction f) : outputFunction_(std::move(f))
    {
    }

    auto operator++ () -> forwarding_iterator&
    {
        return *this;
    }

    auto operator++ (int) -> forwarding_iterator&
    {
        return *this;
    }

    auto operator* () -> forwarding_iterator&
    {
        return *this;
    }

    auto operator= (auto&& arg) -> forwarding_iterator&
    {
        outputFunction_(std::forward<decltype(arg)>(arg));
        return *this;
    }

    auto operator= (auto&& arg) const -> forwarding_iterator const&
    {
        outputFunction_(std::forward<decltype(arg)>(arg));
        return *this;
    }

private:
    OutputFunction outputFunction_;
};
} // namespace teddy::tsl

#endif