#include "iterators.hpp"

#include <libteddy/details/utils.hpp>
#include <algorithm>
#include "expressions.hpp"

namespace teddy
{
// domain_iterator:

    domain_iterator:: domain_iterator
        () :
            domains_ ({}),
            indices_ ({}),
            varVals_ ({})
    {
    }

    domain_iterator::domain_iterator
        (std::vector<uint_t> domains) :
        domain_iterator
            ( std::move(domains)
            , utils::fill_vector(domains.size(), utils::identity)
            , {} )
    {
    }

    domain_iterator::domain_iterator
        (std::vector<uint_t> domains, std::vector<index_t> order) :
        domain_iterator (std::move(domains), std::move(order), {})
    {
    }

    domain_iterator::domain_iterator
        ( std::vector<uint_t>                     domains
        , std::vector<index_t>                    order
        , std::vector<std::pair<index_t, uint_t>> fixed ) :
        domains_ (std::move(domains)),
        indices_ ([&order, &fixed]()
        {
            auto is = std::vector<index_t>();
            std::ranges::copy_if(order, std::back_inserter(is),
                [&fixed](auto const i)
            {
                return std::ranges::end(fixed) == std::ranges::find_if(
                    fixed,
                    [i](auto const p)
                    {
                        return p.first == i;
                    });
            });
            return is;
        }()),
        varVals_ ([this, &fixed, &domains]()
        {
            auto vs = std::vector<uint_t>(domains_.size());
            for (auto const& [i, v] : fixed)
            {
                varVals_[i] = v;
            }
            return vs;
        }())
    {
    }

    auto domain_iterator::operator*
        () const -> std::vector<uint_t> const&
    {
        return varVals_;
    }

    auto domain_iterator::operator++
        () -> domain_iterator&
    {
        auto overflow = false;

        for (auto const i : indices_)
        {
            ++varVals_[i];
            overflow = varVals_[i] == domains_[i];
            if (overflow)
            {
                varVals_[i] = 0;
            }

            if (not overflow)
            {
                break;
            }
        }

        if (overflow)
        {
            domains_.clear();
            indices_.clear();
            varVals_.clear();
        }

        return *this;
    }

    auto domain_iterator::operator++
        (int) -> domain_iterator
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    auto domain_iterator::operator==
        (domain_iterator const& rhs) const -> bool
    {
        return std::ranges::equal(varVals_, rhs.varVals_)
            && std::ranges::equal(indices_, rhs.indices_)
            && std::ranges::equal(domains_, rhs.domains_);
    }

    auto domain_iterator::operator!=
        (domain_iterator const& rhs) const -> bool
    {
        return !(rhs == *this);
    }

    auto domain_iterator::operator==
        (domain_iterator_sentinel) const -> bool
    {
        return varVals_.empty();
    }

    auto domain_iterator::operator!=
        (domain_iterator_sentinel) const -> bool
    {
        return not varVals_.empty();
    }

// evaluating_iterator:

    template<class Expression>
    evaluating_iterator<Expression>::evaluating_iterator
        () :
        iterator_ (),
        expr_     (nullptr)
    {
    }

    template<class Expression>
    evaluating_iterator<Expression>::evaluating_iterator
        (domain_iterator iterator, Expression const& expr) :
        iterator_ (std::move(iterator)),
        expr_     (&expr)
    {
    }

    template<class Expression>
    auto evaluating_iterator<Expression>::operator*
        () const -> uint_t
    {
        return evaluate_expression(*expr_, *iterator_);
    }

    template<class Expression>
    auto evaluating_iterator<Expression>::operator++
        () -> evaluating_iterator&
    {
        ++iterator_;
        return *this;
    }

    template<class Expression>
    auto evaluating_iterator<Expression>::operator++
        (int) -> evaluating_iterator
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    template<class Expression>
    auto evaluating_iterator<Expression>::operator==
        (domain_iterator_sentinel const s) const -> bool
    {
        return iterator_ == s;
    }

    template<class Expression>
    auto evaluating_iterator<Expression>::operator!=
        (domain_iterator_sentinel const s) const -> bool
    {
        return iterator_ != s;
    }

    template<class Expression>
    auto evaluating_iterator<Expression>::var_vals
        () const -> std::vector<uint_t> const&
    {
        return *iterator_;
    }

    template<> class evaluating_iterator<minmax_expr>;
    template<> class evaluating_iterator<expr_node>;

// counting_iterator:

    counting_iterator::counting_iterator
        (std::size_t max) :
        frequency_ (max)
    {
    }

    auto counting_iterator::operator++
        () -> counting_iterator&
    {
        return *this;
    }

    auto counting_iterator::operator++
        (int) -> counting_iterator&
    {
        return *this;
    }

    auto counting_iterator::operator*
        () -> counting_iterator&
    {
        return *this;
    }

    auto counting_iterator::operator=
        (uint_t const v) -> counting_iterator&
    {
        ++frequency_[v];
        return *this;
    }

}