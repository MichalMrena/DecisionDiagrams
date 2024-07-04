#ifndef LIBTEDDY_TSL_EXPRESSION_HPP
#define LIBTEDDY_TSL_EXPRESSION_HPP

#include <libtsl/types.hpp>

#include <memory>
#include <random>
#include <variant>
#include <vector>

namespace teddy::tsl
{
/**
 *  \brief Strong type for vector of terms.
 */
struct minmax_expr
{
    std::vector<std::vector<int32>> terms_;
};

/**
 *  \brief Makes random minmax expression.
 */
auto make_minmax_expression (
    rng_t& indexRng,
    int32 varCount,
    int32 termCount,
    int32 termSize
) -> minmax_expr;

/**
 *  \brief Evaluates \p expr using values of variables in \p vs .
 */
auto evaluate_expression (minmax_expr const& expr, std::vector<int32> const& vs)
    -> int32;

/**
 *  \brief Tags expression node representing variable.
 */
struct expr_node_variable
{
};

/**
 *  \brief Tags expression node representing constant.
 */
struct expr_node_constant
{
};

/**
 *  \brief Tags expression node representing operation.
 */
struct expr_node_operation
{
};

/**
 *  \brief Specifies operation of the operation node.
 */
enum class operation_type
{
    Min,
    Max
};

/**
 *  \brief Node of an expression tree.
 */
class expr_node
{
private:
    struct operation_t
    {
        operation_t(
            operation_type o,
            std::unique_ptr<expr_node> l,
            std::unique_ptr<expr_node> r
        );
        operation_type op_;
        std::unique_ptr<expr_node> l_;
        std::unique_ptr<expr_node> r_;
    };

    struct variable_t
    {
        variable_t(int32 i);
        int32 i_;
    };

    struct constant_t
    {
        constant_t(int32 c);
        int32 c_;
    };

public:
    expr_node(expr_node_variable, int32 i);

    expr_node(expr_node_constant, int32 c);

    expr_node(
        expr_node_operation,
        operation_type o,
        std::unique_ptr<expr_node> l,
        std::unique_ptr<expr_node> r
    );

    auto is_variable () const -> bool;

    auto is_constant () const -> bool;

    auto is_operation () const -> bool;

    auto get_index () const -> int32;

    auto get_value () const -> int32;

    auto evaluate (int32 l, int32 r) const -> int32;

    auto get_left () const -> expr_node const&;

    auto get_right () const -> expr_node const&;

private:
    std::variant<operation_t, variable_t, constant_t> data_;
};

/**
 *  \brief Makes random minmax expression tree.
 */
auto make_expression_tree (
    int32 varcount,
    rng_t& rngtype,
    rng_t& rngbranch
) -> std::unique_ptr<expr_node>;

/**
 *  \brief Evaluates \p expr using values of variables in \p vs .
 */
auto evaluate_expression (expr_node const& expr, std::vector<int32> const& vs)
    -> int32;
} // namespace teddy::tsl

#endif