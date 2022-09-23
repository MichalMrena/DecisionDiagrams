#include "expressions.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

namespace teddy
{
// minmax_expr:

auto generate_minmax_expression(
    std::mt19937_64& indexRng, std::size_t const varCount,
    std::size_t const termCount, std::size_t const termSize
) -> minmax_expr
{
    assert(varCount > 0);
    auto const indexFrom = index_t(0);
    auto const indexTo   = static_cast<index_t>(varCount - 1u);
    auto indexDst = std::uniform_int_distribution<index_t>(indexFrom, indexTo);
    auto terms    = std::vector<std::vector<uint_t>>(termCount);

    for (auto t = 0u; t < termCount; ++t)
    {
        for (auto k = 0u; k < termSize; ++k)
        {
            terms[t].emplace_back(indexDst(indexRng));
        }
    }

    return minmax_expr {std::move(terms)};
}

auto evaluate_expression(minmax_expr const& expr, std::vector<uint_t> const& vs)
    -> uint_t
{
    namespace rs = std::ranges;
    return rs::max(
        expr.terms_ | rs::views::transform(
                          [&vs](auto const& is)
                          {
                              return vs[rs::min(
                                  is, {},
                                  [&vs](auto const i)
                                  {
                                      return vs[i];
                                  }
                              )];
                          }
                      )
    );
}

// expr_node:

expr_node::operation_t::operation_t(
    operation_type o, std::unique_ptr<expr_node> l, std::unique_ptr<expr_node> r
)
    : op_(o), l_(std::move(l)), r_(std::move(r))
{
}

expr_node::variable_t::variable_t(index_t const i)
    : i_(i)
{
}

expr_node::constant_t::constant_t(uint_t const c)
    : c_(c)
{
}

expr_node::expr_node(expr_node_variable, index_t const i)
    : data_(std::in_place_type<variable_t>, i)
{
}

expr_node::expr_node(expr_node_constant, uint_t const c)
    : data_(std::in_place_type<constant_t>, c)
{
}

expr_node::expr_node(
    expr_node_operation, operation_type const o, std::unique_ptr<expr_node> l,
    std::unique_ptr<expr_node> r
)
    : data_(std::in_place_type<operation_t>, o, std::move(l), std::move(r))
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

auto expr_node::get_index() const -> index_t
{
    return std::get<variable_t>(data_).i_;
}

auto expr_node::get_value() const -> uint_t
{
    return std::get<constant_t>(data_).c_;
}

auto expr_node::evaluate(uint_t const l, uint_t const r) const -> uint_t
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

auto generate_expression_tree(
    std::size_t varcount, std::mt19937_64& rngtype, std::mt19937_64& rngbranch
) -> std::unique_ptr<expr_node>
{
    auto go = [&, i = 0u](auto& self, auto const n) mutable
    {
        if (n == 1)
        {
            return std::make_unique<expr_node>(expr_node_variable(), i++);
        }
        else
        {
            auto denomdist     = std::uniform_int_distribution<uint_t>(2, 10);
            auto typedist      = std::uniform_real_distribution(0.0, 1.0);
            auto const denom   = denomdist(rngbranch);
            auto const lhssize = std::max(1ul, n / denom);
            auto const rhssize = n - lhssize;
            auto const p       = typedist(rngtype);
            auto const op = p < 0.5 ? operation_type::Min : operation_type::Max;
            return std::make_unique<expr_node>(
                expr_node_operation(), op, self(self, lhssize),
                self(self, rhssize)
            );
        }
    };
    return go(go, varcount);
}

auto evaluate_expression(expr_node const& expr, std::vector<uint_t> const& vs)
    -> uint_t
{
    auto const go = [&vs](auto self, auto const& node)
    {
        if (node.is_variable())
        {
            return vs[node.get_index()];
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
} // namespace teddy