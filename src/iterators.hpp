#ifndef TEDDY_TESTS_ITERATORS_HPP
#define TEDDY_TESTS_ITERATORS_HPP

#include <algorithm>
#include <ranges>
#include <vector>
#include <libteddy/details/types.hpp>
#include <libteddy/details/utils.hpp>

using teddy::int32;
using teddy::int64;

using teddy::as_uindex;

struct domain_iterator_sentinel
{
};

class domain_iterator
{
public:
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::vector<int32>;
    using pointer           = value_type*;
    using reference         = value_type&;
    using iterator_category = std::input_iterator_tag;

public:
    domain_iterator() : domains_({}), indices_({}), varVals_({})
    {
    }

    domain_iterator(std::vector<int32> domains)
        : domain_iterator(
            std::move(domains),
            teddy::utils::fill_vector(ssize(domains), teddy::utils::identity),
            {}
        )
    {
    }

    domain_iterator(std::vector<int32> domains, std::vector<int32> order)
        : domain_iterator(std::move(domains), std::move(order), {})
    {
    }

    domain_iterator(
        std::vector<int32> domains,
        std::vector<int32> order,
        std::vector<std::pair<int32, int32>> fixed
    )
        : domains_(std::move(domains)),
        indices_(
            [&order, &fixed]()
            {
                auto is = std::vector<int32>();
                std::ranges::copy_if(
                    order,
                    std::back_inserter(is),
                    [&fixed](auto const i)
                    {
                        return std::ranges::end(fixed) ==
                                std::ranges::find_if(
                                    fixed,
                                    [i](auto const p)
                                    {
                                        return p.first == i;
                                    }
                                );
                    }
                );
                std::ranges::reverse(is);
                return is;
            }()
        ),
        varVals_(
            [this, &fixed, &domains]()
            {
                auto vs = std::vector<int32>(domains_.size());
                for (auto const& [i, v] : fixed)
                {
                    varVals_[as_uindex(i)] = v;
                }
                return vs;
            }()
        )
    {

    }

    auto operator*() const -> std::vector<int32> const&
    {
        return varVals_;
    }

    auto operator++() -> domain_iterator&
    {
        auto overflow = false;

        for (auto const i : indices_)
        {
            ++varVals_[as_uindex(i)];
            overflow = varVals_[as_uindex(i)] == domains_[as_uindex(i)];
            if (overflow)
            {
                varVals_[as_uindex(i)] = 0;
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

    auto operator++(int) -> domain_iterator
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    auto operator==(domain_iterator const& rhs) const -> bool
    {
        return std::ranges::equal(varVals_, rhs.varVals_) &&
            std::ranges::equal(indices_, rhs.indices_) &&
            std::ranges::equal(domains_, rhs.domains_);
    }

    auto operator!=(domain_iterator const& rhs) const -> bool
    {
        return ! (rhs == *this);
    }

    auto operator==(domain_iterator_sentinel) const -> bool
    {
        return varVals_.empty();
    }

    auto operator!=(domain_iterator_sentinel) const -> bool
    {
        return not varVals_.empty();
    }

protected:
    std::vector<int32> domains_;
    std::vector<int32> indices_;
    std::vector<int32> varVals_;
};

#endif