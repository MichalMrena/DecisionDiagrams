#ifndef TEDDY_SRC_TREES_HPP
#define TEDDY_SRC_TREES_HPP

#include <algorithm>
#include <libteddy/teddy.hpp>
#include <limits>
#include <string>
#include <variant>
#include <vector>

using teddy::int32;
using teddy::int64;

/**
 *  @brief Type of an operation in an internal node.
 */
enum class Operation
{
    And,
    Or,
    Undefined
};

/**
 *  @brief returns neutral element for the operation @p o .
 */
auto get_neutral_element(Operation o) -> int32;

/**
 *  @brief returns string representaiton of the operation @p o .
 */
auto to_string (Operation o) -> std::string;

/**
 * @brief Leaf node hoding index of a variable.
 */
struct LeafNode
{
    int32 index_;
};

// Forward declarations:

struct BinaryNode;
struct MultiwayNode;

/**
 *  @brief Internal binary operation node.
 */
struct BinOpNode
{
    Operation op_;
    BinaryNode const* lhs_;
    BinaryNode const* rhs_;

    auto evaluate(int32 l, int32 r) const -> int32;
};

/**
 *  @brief Internal n-ary operation node.
 */
struct NAryOpNode
{
    Operation op_;
    std::vector<MultiwayNode*> args_;

    auto evaluate(std::vector<int32> const& args) const -> int32;
};

/**
 *  @brief Node of a binary AST.
 */
struct BinaryNode
{
    std::variant<std::monostate, LeafNode, BinOpNode> data_;

    auto is_variable() const -> bool;

    auto is_constant() const -> bool;

    auto is_operation() const -> bool;

    auto get_index() const -> int32;

    auto get_value() const -> int32;

    auto get_operation() const -> Operation;

    auto evaluate(int32 l, int32 r) const -> int32;

    auto get_left() const -> BinaryNode const&;

    auto get_right() const -> BinaryNode const&;
};

static_assert(
    teddy::expression_node<BinaryNode>,
    "BinaryNode satisfies expression_node"
);

/**
 *  @brief Generic DFS traversal of a binary AST.
 *
 *  Invokes @p f on each node providing reference to the node,
 *  unique id of the node, and unique id of the parent node.
 *
 *  @tparam F :: BinaryNode const& -> Int -> Int -> ()
 */
template<class F>
auto for_each_dfs (BinaryNode const& root, F f) -> void
{
    auto nextId = 0;
    auto go = [&nextId, f](
        auto self,
        BinaryNode const& node,
        int const parentId
    ) -> void
    {
        auto const thisId = nextId;
        ++nextId;
        std::invoke(f, node, parentId, thisId);
        if (node.is_operation())
        {
            self(self, node.get_left(), thisId);
            self(self, node.get_right(), thisId);
        }
    };
    go(go, root, -1);
}

/**
 *  @brief Node of a multiway AST.
 */
struct MultiwayNode
{
    int64 id_;
    std::variant<std::monostate, LeafNode, NAryOpNode> data_;

    auto is_variable() const -> bool;

    auto is_operation() const -> bool;

    auto get_index() const -> int32;

    auto get_operation() const -> Operation;

    auto evaluate(std::vector<int32> const& args) const -> int32;

    auto get_args() const -> std::vector<MultiwayNode*> const&;

    auto as_opnode() -> NAryOpNode&;

    auto as_leafnode() -> LeafNode&;

    auto as_opnode() const -> NAryOpNode const&;

    auto as_leafnode() const -> LeafNode const&;
};

/**
 *  @brief Checks if @p node is internal node with at least one son.
 */
auto has_leaf_son (MultiwayNode const& node) -> bool;

/**
 *  @brief Returns the number of sons of @p node that are leaves.
 */
auto leaf_son_count (MultiwayNode const& node) -> int64;

/**
 *  @brief Returns the number of leaves in the tree.
 */
auto leaf_count (MultiwayNode const& root) -> int64;

/**
 *  @brief Creates deep copy of the tree with @p root .
 */
auto copy_tree (MultiwayNode const& root) -> MultiwayNode*;

/**
 *  @brief Function object type for hash of a multiway node.
 */
struct MwNodeHash
{
    [[nodiscard]]
    auto operator()(MultiwayNode const& node) const -> std::size_t
    {
        if (node.is_variable())
        {
            return std::size_t{1};
        }
        else
        {
            auto result = std::size_t{0};
            for (auto const* son : node.get_args())
            {
                auto const hash = std::hash<MultiwayNode const*>()(son);
                result ^= hash + 0x9e3779b9 + (result << 6) + (result >> 2);
            }
            return result;
        }
    }
};

/**
 *  @brief Function object type for comparison of two multiway nodes.
 */
struct MwNodeEquals
{
    [[nodiscard]]
    auto operator()(MultiwayNode const& l, MultiwayNode const& r) const -> bool
    {
        if (l.is_variable() && r.is_variable())
        {
            return true;
        }
        else if (l.is_variable() || r.is_variable())
        {
            return false;
        }
        else
        {
            return l.get_operation() == r.get_operation()
                && std::ranges::equal(l.get_args(), r.get_args());
        }
    }
};

/**
 *  @brief Generic DFS traversal of a binary AST.
 *
 *  Invokes @p f on each node providing reference to the node,
 *  unique id of the node, and unique id of the parent node.
 *
 *  @tparam F :: MultiwayNode const& -> Int -> Int -> ()
 */
template<class F>
auto for_each_dfs(MultiwayNode const& root, F f)
{
    auto nextId = 0;
    auto go = [&nextId, f](
        auto self,
        MultiwayNode const& node,
        int64 const parentId
    ) -> void
    {
        auto const thisId = nextId;
        ++nextId;
        std::invoke(f, node, parentId, thisId);
        if (node.is_operation())
        {
            for (auto* son : node.get_args())
            {
                self(self, *son, thisId);
            }
        }
    };
    go(go, root, -1);
}

/**
 *  @brief Dumps binary AST into dot format.
 */
auto dump_dot (BinaryNode const& root) -> std::string;

/**
 *  @brief Dumps multiway AST into dot format. Ignores indices and ops.
 */
auto dump_dot (MultiwayNode const& root) -> std::string;

class SeriesParallelGenerator;

/**
 *  @brief Dumps multiway AST into dot format. Considers ops and indices.
 */
auto dump_dot (
    MultiwayNode const& root,
    SeriesParallelGenerator const& gen
) -> std::string;

#endif