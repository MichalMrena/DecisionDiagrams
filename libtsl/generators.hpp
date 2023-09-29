#ifndef LIBTEDDY_TSL_GENERATORS_HPP
#define LIBTEDDY_TSL_GENERATORS_HPP

#include <libteddy/core.hpp>
#include <libtsl/expressions.hpp>

namespace teddy::tsl
{
    template<class Dat, class Deg, class Dom>
    auto make_diagram (
        teddy::tsl::minmax_expr const& expr,
        teddy::diagram_manager<Dat, Deg, Dom>& manager
    )
    {
        using mdd = typename teddy::diagram_manager<Dat, Deg, Dom>::diagram_t;
        auto termDs     = std::vector<mdd>();
        for (auto const& eTerm : expr.terms_)
        {
            auto vars = manager.variables(eTerm);
            termDs.emplace_back(manager.template left_fold<teddy::ops::MIN>(
                vars
            ));
        }
        return manager.template tree_fold<teddy::ops::MAX>(termDs);
    }
}

#endif