#include "trees.hpp"

#include <numeric>

auto get_neutral_element(Operation const o) -> int32
{
    switch (o)
    {
        case Operation::And: return std::numeric_limits<int32>::max();
        case Operation::Or:  return std::numeric_limits<int32>::min();
        default:      unreachable(); return int32{};
    }
}

auto to_string (Operation const o) -> std::string
{
    switch (o)
    {
        case Operation::And:           return "and";
        case Operation::Or:            return "or";
        case Operation::Undefined:     return "op";
        default: unreachable(); return "";
    }
}

auto BinOpNode::evaluate(int32 const l, int32 const r) const -> int32
{
    switch (op_)
    {
    case Operation::And:
        return std::min(l, r);

    case Operation::Or:
        return std::max(l, r);

    default:
        unreachable(); return int32{};
    }
}

auto NAryOpNode::evaluate(std::vector<int32> const& args) const -> int32
{
    switch (op_)
    {
        case Operation::And:
            return std::reduce(
                begin(args),
                end(args),
                get_neutral_element(op_),
                teddy::ops::MIN()
            );

        case Operation::Or:
            return std::reduce(
                begin(args),
                end(args),
                get_neutral_element(op_),
                teddy::ops::MAX()
            );

        default:
            unreachable();
            return int32{};
    }
}

auto BinaryNode::is_variable() const -> bool
{
    return std::holds_alternative<LeafNode>(data_);
}

auto BinaryNode::is_constant() const -> bool
{
    return false;
}

auto BinaryNode::is_operation() const -> bool
{
    return std::holds_alternative<BinOpNode>(data_);
}

auto BinaryNode::get_index() const -> int32
{
    return std::get<LeafNode>(data_).index_;
}

auto BinaryNode::get_value() const -> int32
{
    return int32{};
}

auto BinaryNode::get_operation() const -> Operation
{
    return std::get<BinOpNode>(data_).op_;
}

auto BinaryNode::evaluate(int32 const l, int32 const r) const -> int32
{
    return std::get<BinOpNode>(data_).evaluate(l, r);
}

auto BinaryNode::get_left() const -> BinaryNode const&
{
    return *std::get<BinOpNode>(data_).lhs_;
}

auto BinaryNode::get_right() const -> BinaryNode const&
{
    return *std::get<BinOpNode>(data_).rhs_;
}

auto MultiwayNode::is_variable() const -> bool
{
    return std::holds_alternative<LeafNode>(data_);
}

auto MultiwayNode::is_operation() const -> bool
{
    return std::holds_alternative<NAryOpNode>(data_);
}

auto MultiwayNode::get_index() const -> int32
{
    return std::get<LeafNode>(data_).index_;
}

auto MultiwayNode::get_operation() const -> Operation
{
    return std::get<NAryOpNode>(data_).op_;
}

auto MultiwayNode::evaluate(std::vector<int32> const& args) const -> int32
{
    return std::get<NAryOpNode>(data_).evaluate(args);
}

auto MultiwayNode::get_args() const -> std::vector<MultiwayNode*> const&
{
    return std::get<NAryOpNode>(data_).args_;
}

auto MultiwayNode::as_opnode() const -> NAryOpNode const&
{
    return std::get<NAryOpNode>(data_);
}

auto MultiwayNode::as_leafnode() const -> LeafNode const&
{
    return std::get<LeafNode>(data_);
}

// dump_dot_impl :: BinaryNode | MultiwayNode -> std::string
auto dump_dot_impl (auto const& root) -> std::string
{
    auto out = std::string();
    out += "digraph BinTree {\n";

    auto s = [](auto const x){ return std::to_string(x); };

    for_each_dfs(root, [&](
        auto const& node,
        int const,
        int const nodeId
    )
    {
        auto const label = node.is_variable()
            ? "x"
            : to_string(node.get_operation());
        out += "    " + s(nodeId) + " [label=\"" + label + "\"];\n";
    });
    out += "\n";

    for_each_dfs(root, [&out, s](
        auto const&,
        int const parentId,
        int const nodeId
    )
    {
        if (parentId != -1)
        {
            out += "    "
                + s(parentId)
                + " -> "
                + s(nodeId) + ";\n";
        }
    });
    out += "}\n";

    return out;
}

auto dump_dot (BinaryNode const& root) -> std::string
{
    return dump_dot_impl(root);
}

auto dump_dot (MultiwayNode const& root) -> std::string
{
    return dump_dot_impl(root);
}