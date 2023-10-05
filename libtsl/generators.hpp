#ifndef LIBTEDDY_TSL_GENERATORS_HPP
#define LIBTEDDY_TSL_GENERATORS_HPP

#include <libteddy/core.hpp>
#include <libtsl/expressions.hpp>

namespace teddy::tsl
{
    template<class Dat, class Deg, class Dom>
    auto make_diagram (
        minmax_expr const& expr,
        diagram_manager<Dat, Deg, Dom>& manager
    )
    {
        using mdd = typename diagram_manager<Dat, Deg, Dom>::diagram_t;
        std::vector<mdd> terms;
        for (auto const& eTerm : expr.terms_)
        {
            auto vars = manager.variables(eTerm);
            terms.push_back(manager.template left_fold<ops::MIN>(vars));
        }
        return manager.template tree_fold<ops::MAX>(terms);
    }

    template<class Dat, class Deg, class Dom>
    auto make_diagram (
        std::unique_ptr<tsl::expr_node> const& expr,
        diagram_manager<Dat, Deg, Dom>& manager
    )
    {
        return manager.from_expression_tree(*expr);
    }
}

#endif