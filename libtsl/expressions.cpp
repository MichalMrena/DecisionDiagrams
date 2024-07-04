#include "expressions.hpp"

#include <algorithm>
#include <cassert>
#include <limits>

namespace teddy::tsl
{
// minmax_expr:

auto make_minmax_expression (
    rng_t& indexRng,
    int32 const varCount,
    int32 const termCount,
    int32 const termSize
) -> minmax_expr
{
    assert(varCount > 0);
    auto const indexFrom = 0;
    auto const indexTo   = varCount - 1;
    auto indexDst = std::uniform_int_distribution<int32>(indexFrom, indexTo);
    auto terms    = std::vector<std::vector<int32>>(as_usize(termCount));

    for (auto t = 0; t < termCount; ++t)
    {
        for (auto k = 0; k < termSize; ++k)
        {
            terms[as_uindex(t)].emplace_back(indexDst(indexRng));
        }
    }

    return minmax_expr {std::move(terms)};
}

auto evaluate_expression (minmax_expr const& expr, std::vector<int32> const& vs)
    -> int32
{
    auto totalMaxVal = std::numeric_limits<int32>::min();
    for (auto const& term : expr.terms_)
    {
        auto termMinVal = std::numeric_limits<int32>::max();
        for (auto const var : term)
        {
            auto const val = vs[as_uindex(var)];
            termMinVal     = val < termMinVal ? val : termMinVal;
        }
        totalMaxVal = termMinVal > totalMaxVal ? termMinVal : totalMaxVal;
    }
    return totalMaxVal;
}

// expr_node:

expr_node::operation_t::operation_t(
    operation_type o,
    std::unique_ptr<expr_node> l,
    std::unique_ptr<expr_node> r
) :
    op_(o),
    l_(std::move(l)),
    r_(std::move(r))
{
}

expr_node::variable_t::variable_t(int32 const i) : i_(i)
{
}

expr_node::constant_t::constant_t(int32 const c) : c_(c)
{
}

expr_node::expr_node(expr_node_variable, int32 const i) :
    data_(std::in_place_type<variable_t>, i)
{
}

expr_node::expr_node(expr_node_constant, int32 const c) :
    data_(std::in_place_type<constant_t>, c)
{
}

expr_node::expr_node(
    expr_node_operation,
    operation_type const o,
    std::unique_ptr<expr_node> l,
    std::unique_ptr<expr_node> r
) :
    data_(std::in_place_type<operation_t>, o, std::move(l), std::move(r))
{
}

auto expr_node::is_variable() const -> bool
{
    return std::holds_alternative<variable_t>(data_);
}

auto expr_node::is_constant() const -> bool
{
    return std::holds_alternative<constant_t>(data_);
}

auto expr_node::is_operation() const -> bool
{
    return std::holds_alternative<operation_t>(data_);
}

auto expr_node::get_index() const -> int32
{
    return std::get<variable_t>(data_).i_;
}

auto expr_node::get_value() const -> int32
{
    return std::get<constant_t>(data_).c_;
}

auto expr_node::evaluate(int32 const l, int32 const r) const -> int32
{
    switch (std::get<operation_t>(data_).op_)
    {
    case operation_type::Min:
        return l < r ? l : r;

    case operation_type::Max:
        return l < r ? r : l;

    default:
        assert(false);
        return 0;
    }
}

auto expr_node::get_left() const -> expr_node const&
{
    return *std::get<operation_t>(data_).l_;
}

auto expr_node::get_right() const -> expr_node const&
{
    return *std::get<operation_t>(data_).r_;
}

auto make_expression_tree (
    int32 varcount,
    rng_t& rngtype,
    rng_t& rngbranch
) -> std::unique_ptr<expr_node>
{
    auto go = [&, i = 0u] (auto& self, auto const n) mutable
    {
        if (n == 1)
        {
            return std::make_unique<expr_node>(expr_node_variable(), i++);
        }
        else
        {
            auto denomdist     = std::uniform_int_distribution<int32>(2, 10);
            auto typedist      = std::uniform_real_distribution(0.0, 1.0);
            auto const denom   = denomdist(rngbranch);
            auto const lhssize = std::max(1, n / denom);
            auto const rhssize = n - lhssize;
            auto const p       = typedist(rngtype);
            auto const op = p < 0.5 ? operation_type::Min : operation_type::Max;
            return std::make_unique<expr_node>(
                expr_node_operation(),
                op,
                self(self, lhssize),
                self(self, rhssize)
            );
        }
    };
    return go(go, varcount);
}

auto evaluate_expression (expr_node const& expr, std::vector<int32> const& vs)
    -> int32
{
    auto const go = [&vs] (auto self, auto const& node)
    {
        if (node.is_variable())
        {
            return vs[as_uindex(node.get_index())];
        }
        else if (node.is_constant())
        {
            return node.get_value();
        }
        else
        {
            assert(node.is_operation());
            auto const l = self(self, node.get_left());
            auto const r = self(self, node.get_right());
            return node.evaluate(l, r);
        }
    };
    return go(go, expr);
}
} // namespace teddy::tsl