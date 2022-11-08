#ifndef LIBTEDDY_TESTS_ITERATORS_HPP
#define LIBTEDDY_TESTS_ITERATORS_HPP

#include "expressions.hpp"
#include <vector>

namespace teddy
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
    using value_type        = std::vector<uint_t>;
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
     *  @p domains of individual variables
     */
    domain_iterator(std::vector<uint_t> domains);

    /**
     *  \brief Initializes using explicitly provided order.
     *
     *  Uses order of variables defined in @p order . Variable with
     *  index @c order[0] changes most often, then variable with
     *  index @c order[1] and so on...
     *  @p domains of individual variables
     *  @p order   order in which variables are incremented
     */
    domain_iterator(std::vector<uint_t> domains, std::vector<index_t> order);

    /**
     *  \brief Initializes using explicitly provided order and fixed values.
     *
     *  Uses order of variables defined in @p order . Variable with
     *  index @c order[0] changes most often, then variable with
     *  index @c order[1] and so on... while skipping variables
     *  defined as fixed by @p fixed .
     *  @p domains of individual variables
     *  @p order   order in which variables are incremented
     *  @p fixed   defines variables with fixed value
     */
    domain_iterator(
        std::vector<uint_t> domains,
        std::vector<index_t> order,
        std::vector<std::pair<index_t, uint_t>> fixed
    );

    auto operator*() const -> std::vector<uint_t> const&;

    auto operator++() -> domain_iterator&;

    auto operator++(int) -> domain_iterator;

    auto operator==(domain_iterator const& rhs) const -> bool;

    auto operator!=(domain_iterator const& rhs) const -> bool;

    auto operator==(domain_iterator_sentinel) const -> bool;

    auto operator!=(domain_iterator_sentinel) const -> bool;

protected:
    std::vector<uint_t> domains_;
    std::vector<index_t> indices_;
    std::vector<uint_t> varVals_;
};

/**
 *  \brief Sentinel for evaluating iterator.
 */
struct evaluating_iterator_sentinel
{
};

/**
 *  \brief Iterator that evaluates expression over a domain.
 */
template<class Expression>
class evaluating_iterator
{
public:
    using difference_type   = std::ptrdiff_t;
    using value_type        = uint_t;
    using pointer           = value_type*;
    using reference         = value_type&;
    using iterator_category = std::input_iterator_tag;

public:
    evaluating_iterator();

    evaluating_iterator(domain_iterator iterator, Expression const& expr);

    auto operator*() const -> uint_t;

    auto operator++() -> evaluating_iterator&;

    auto operator++(int) -> evaluating_iterator;

    auto operator==(evaluating_iterator_sentinel const s) const -> bool;

    auto operator!=(evaluating_iterator_sentinel const s) const -> bool;

    auto var_vals() const -> std::vector<uint_t> const&;

private:
    domain_iterator iterator_;
    Expression const* expr_;
};

/**
 *  \brief Proxy output iterator that feeds outputed values into function.
 */
template<class F>
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
    forwarding_iterator(F& f) : f_(&f)
    {
    }

    auto operator++() -> forwarding_iterator&
    {
        return *this;
    }

    auto operator++(int) -> forwarding_iterator&
    {
        return *this;
    }

    auto operator*() -> forwarding_iterator&
    {
        return *this;
    }

    auto operator=(auto&& arg) -> forwarding_iterator&
    {
        (*f_)(std::forward<decltype(arg)>(arg));
        return *this;
    }

    auto operator=(auto&& arg) const -> forwarding_iterator const&
    {
        (*f_)(std::forward<decltype(arg)>(arg));
        return *this;
    }

private:
    F* f_ {nullptr};
};
} // namespace teddy

#endif