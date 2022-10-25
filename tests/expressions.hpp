#ifndef LIBTEDDY_TESTS_EXPRESSION_HPP
#define LIBTEDDY_TESTS_EXPRESSION_HPP

#include <libteddy/details/types.hpp>
#include <memory>
#include <random>
#include <variant>
#include <vector>

namespace teddy
{
/**
 *  \brief Strong type for vector of terms.
 */
struct minmax_expr
{
    std::vector<std::vector<uint_t>> terms_;
};

/**
 *  \brief Makes random minmax expression.
 */
auto make_minmax_expression(
    std::mt19937_64& indexRng, std::size_t varCount, std::size_t termCount,
    std::size_t termSize
) -> minmax_expr;

/**
 *  \brief Evaluates \p expr using values of variables in \p vs .
 */
auto evaluate_expression(minmax_expr const& expr, std::vector<uint_t> const& vs)
    -> uint_t;

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
            operation_type o, std::unique_ptr<expr_node> l,
            std::unique_ptr<expr_node> r
        );
        operation_type op_;
        std::unique_ptr<expr_node> l_;
        std::unique_ptr<expr_node> r_;
    };

    struct variable_t
    {
        variable_t(index_t i);
        index_t i_;
    };

    struct constant_t
    {
        constant_t(uint_t c);
        uint_t c_;
    };

public:
    expr_node(expr_node_variable, index_t i);

    expr_node(expr_node_constant, uint_t c);

    expr_node(
        expr_node_operation, operation_type o, std::unique_ptr<expr_node> l,
        std::unique_ptr<expr_node> r
    );

    auto is_variable() const -> bool;

    auto is_constant() const -> bool;

    auto is_operation() const -> bool;

    auto get_index() const -> index_t;

    auto get_value() const -> uint_t;

    auto evaluate(uint_t l, uint_t r) const -> uint_t;

    auto get_left() const -> expr_node const&;

    auto get_right() const -> expr_node const&;

private:
    std::variant<operation_t, variable_t, constant_t> data_;
};

/**
 *  \brief Makes random minmax expression tree.
 */
auto make_expression_tree(
    std::size_t varcount, std::mt19937_64& rngtype, std::mt19937_64& rngbranch
) -> std::unique_ptr<expr_node>;

/**
 *  \brief Evaluates \p expr using values of variables in \p vs .
 */
auto evaluate_expression(expr_node const& expr, std::vector<uint_t> const& vs)
    -> uint_t;
} // namespace teddy

#endif