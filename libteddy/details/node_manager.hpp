#ifndef LIBTEDDY_DETAILS_NODE_MANAGER_HPP
#define LIBTEDDY_DETAILS_NODE_MANAGER_HPP

#include <libteddy/details/debug.hpp>
#include <libteddy/details/hash_tables.hpp>
#include <libteddy/details/node.hpp>
#include <libteddy/details/node_pool.hpp>
#include <libteddy/details/operators.hpp>
#include <libteddy/details/tools.hpp>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <ostream>
#include <queue>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace teddy
{
namespace domains
{
struct mixed
{
    std::vector<int32> domains_;

    mixed(std::vector<int32> domains) : domains_(std::move(domains)) {};

    auto operator[] (int32 const index) const
    {
        return domains_[as_uindex(index)];
    }
};

template<int32 N>
struct fixed
{
    static_assert(N > 1);

    constexpr auto operator[] ([[maybe_unused]] int32 const index) const
    {
        return N;
    }

    constexpr auto operator() () const
    {
        return N;
    }
};

template<class T>
struct is_fixed : public std::false_type
{
};

template<int32 N>
struct is_fixed<fixed<N>> : public std::true_type
{
};

template<class T>
using is_mixed = std::is_same<T, mixed>;
} // namespace domains

template<class T>
concept domain = domains::is_mixed<T>()() || domains::is_fixed<T>()();

template<class F>
concept int_to_int = requires(F function, int32 value) {
                         {
                             function(value)
                         } -> std::convertible_to<int32>;
                     };

template<class F, class Node>
concept node_op = requires(F function, Node* node) { function(node); };

template<class Data, degree Degree, domain Domain>
class node_manager
{
public:
    using node_t        = node<Data, Degree>;
    using son_container = typename node_t::son_container;

    struct common_init_tag
    {
    };

public:
    node_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        std::vector<int32> order
    )
    requires(domains::is_fixed<Domain>()());

    node_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        std::vector<int32> order,
        domains::mixed domains
    )
    requires(domains::is_mixed<Domain>()());

    node_manager(node_manager&&) noexcept = default;
    ~node_manager()                       = default;
    node_manager(node_manager const&)     = delete;
    auto operator= (node_manager const&)  = delete;
    auto operator= (node_manager&&)       = delete;

    auto set_cache_ratio (double ratio) -> void;
    auto set_gc_ratio (double ratio) -> void;
    auto set_auto_reorder (bool doReorder) -> void;

private:
    node_manager(
        common_init_tag initTag,
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        std::vector<int32> order,
        Domain domains
    );

public:
    [[nodiscard]] auto get_terminal_node (int32 value) const -> node_t*;
    [[nodiscard]] auto make_terminal_node (int32 value) -> node_t*;
    [[nodiscard]] auto make_special_node (int32 value) -> node_t*;
    [[nodiscard]] auto make_internal_node (int32 index, son_container&& sons)
        -> node_t*;
    [[nodiscard]] auto get_level (int32 index) const -> int32;
    [[nodiscard]] auto get_level (node_t* node) const -> int32;
    [[nodiscard]] auto get_leaf_level () const -> int32;
    [[nodiscard]] auto get_index (int32 level) const -> int32;
    [[nodiscard]] auto get_domain (int32 index) const -> int32;
    [[nodiscard]] auto get_node_count (int32 index) const -> int64;
    [[nodiscard]] auto get_node_count (node_t* node) const -> int64;
    [[nodiscard]] auto get_node_count () const -> int64;
    [[nodiscard]] auto get_var_count () const -> int32;
    [[nodiscard]] auto get_order () const -> std::vector<int32> const&;
    [[nodiscard]] auto get_domains () const -> std::vector<int32>;
    auto force_gc () -> void;

    auto to_dot_graph (std::ostream& ost) const -> void;
    auto to_dot_graph (std::ostream& ost, node_t* node) const -> void;

    [[nodiscard]] auto domain_product (int32 levelFrom, int32 levelTo) const
        -> int64;

    template<utils::i_gen Generator>
    auto make_sons (int32 index, Generator&& generator) -> son_container;

    template<node_op<node<Data, Degree>> NodeOp>
    auto for_each_son (node_t* node, NodeOp&& operation) const -> void;

    template<node_op<node<Data, Degree>> NodeOp>
    auto for_each_son (
        int32 index,
        son_container const& sons,
        NodeOp&& operation
    ) const -> void;

    template<node_op<node<Data, Degree>> NodeOp>
    auto for_each_node (NodeOp&& operation) const -> void;

    template<node_op<node<Data, Degree>> NodeOp>
    auto for_each_terminal_node (NodeOp&& operation) const -> void;

    template<teddy_bin_op O>
    [[nodiscard]] auto cache_find (node_t* lhs, node_t* rhs) -> node_t*;

    template<teddy_bin_op O>
    auto cache_put (node_t* result, node_t* lhs, node_t* rhs) -> void;

    template<node_op<node<Data, Degree>> NodeOp>
    auto traverse_pre (node_t* rootNode, NodeOp&& nodeOp) const -> void;

    template<node_op<node<Data, Degree>> NodeOp>
    auto traverse_post (node_t* rootNode, NodeOp&& nodeOp) const -> void;

    template<node_op<node<Data, Degree>> NodeOp>
    auto traverse_level (node_t* rootNode, NodeOp&& nodeOp) const -> void;

    [[nodiscard]] auto is_valid_var_value (int32 index, int32 value) const
        -> bool;

    auto run_deferred () -> void;

    static auto dec_ref_count (node_t* node) -> void;

    auto sift_variables () -> void;

private:
    [[nodiscard]] auto is_redundant (int32 index, son_container const& sons)
        const -> bool;

    auto adjust_tables () -> void;
    auto adjust_caches () -> void;

    auto swap_variable_with_next (int32 index) -> void;
    auto swap_node_with_next (node_t* node) -> void;
    auto dec_ref_try_gc (node_t* node) -> void;

    template<class... Args>
    [[nodiscard]] auto make_new_node (Args&&... args) -> node_t*;
    auto delete_node (node_t* node) -> void;

    auto traverse_no_op (node_t* rootNode) const -> void;

    template<class ForEachNode>
    auto to_dot_graph_common (std::ostream& ost, ForEachNode&& forEach) const
        -> void;

    auto deferr_gc_reorder () -> void;

    auto collect_garbage () -> void;

    [[nodiscard]] static auto check_distinct (std::vector<int32> const& ints)
        -> bool;

    [[nodiscard]] static auto can_be_gced (node_t* node) -> bool;

private:
    apply_cache<Data, Degree> opCache_;
    node_pool<Data, Degree> pool_;
    std::vector<unique_table<Data, Degree>> uniqueTables_;
    std::vector<node_t*> terminals_;
    std::vector<node_t*> specials_;
    std::vector<int32> indexToLevel_;
    std::vector<int32> levelToIndex_;
    [[no_unique_address]] Domain domains_;
    int64 nodeCount_;
    double cacheRatio_;
    double gcRatio_;
    int64 nextTableAdjustment_;
    bool autoReorderEnabled_;
    bool gcReorderDeferred_;
};

template<class Data, degree D>
auto id_inc_ref_count (node<Data, D>* const node) -> ::teddy::node<Data, D>*
{
    node->inc_ref_count();
    return node;
}

template<class Data, degree D>
auto id_set_marked (node<Data, D>* const node) -> ::teddy::node<Data, D>*
{
    node->set_marked();
    return node;
}

template<class Data, degree D>
auto id_set_notmarked (node<Data, D>* const node) -> ::teddy::node<Data, D>*
{
    node->set_notmarked();
    return node;
}

template<class Data, degree Degree, domain Domain>
node_manager<Data, Degree, Domain>::node_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> order
)
requires(domains::is_fixed<Domain>()())
    :
    node_manager(
        common_init_tag(),
        varCount,
        nodePoolSize,
        overflowNodePoolSize,
        std::move(order),
        {}
    )
{
}

template<class Data, degree Degree, domain Domain>
node_manager<Data, Degree, Domain>::node_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> order,
    domains::mixed domains
)
requires(domains::is_mixed<Domain>()())
    :
    node_manager(
        common_init_tag(),
        varCount,
        nodePoolSize,
        overflowNodePoolSize,
        std::move(order),
        [&] () -> decltype(auto)
        {
            assert(ssize(domains.domains_) == varCount);
            return std::move(domains);
        }()
    )
{
}

template<class Data, degree Degree, domain Domain>
node_manager<Data, Degree, Domain>::node_manager(
    [[maybe_unused]] common_init_tag initTag,
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> order,
    Domain domains
) :
    opCache_(),
    pool_(nodePoolSize, overflowNodePoolSize),
    uniqueTables_(as_usize(varCount)),
    terminals_({}),
    specials_({}),
    indexToLevel_(as_usize(varCount)),
    levelToIndex_(std::move(order)),
    domains_(std::move(domains)),
    nodeCount_(0),
    cacheRatio_(0.5),
    gcRatio_(0.05),
    nextTableAdjustment_(230),
    autoReorderEnabled_(false),
    gcReorderDeferred_(false)
{
    assert(ssize(levelToIndex_) == this->get_var_count());
    assert(check_distinct(levelToIndex_));
    if constexpr (domains::is_mixed<Domain>()() && degrees::is_fixed<Degree>()())
    {
        for ([[maybe_unused]] auto const domain : domains_.domains_)
        {
            assert(domain <= Degree()());
        }
    }

    // Create reverse mapping
    // from (level -> index)
    // to   (index -> level).
    auto level = 0;
    for (auto const index : levelToIndex_)
    {
        indexToLevel_[as_uindex(index)] = level++;
    }
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::set_cache_ratio(double const ratio)
    -> void
{
    assert(ratio > 0);
    cacheRatio_ = ratio;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::set_gc_ratio(double const ratio)
    -> void
{
    assert(ratio >= 0.0 && ratio <= 1.0);
    gcRatio_ = ratio;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::set_auto_reorder(bool const doReorder)
    -> void
{
    autoReorderEnabled_ = doReorder;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_terminal_node(int32 const value
) const -> node_t*
{
    return value < ssize(terminals_) ? terminals_[as_uindex(value)] : nullptr;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::make_terminal_node(int32 const value)
    -> node_t*
{
    if (is_special(value))
    {
        return this->make_special_node(value);
    }

    if constexpr (domains::is_fixed<Domain>()())
    {
        assert(value < Domain()());
    }

    if (value >= ssize(terminals_))
    {
        terminals_.resize(as_usize(value + 1), nullptr);
    }

    if (not terminals_[as_uindex(value)])
    {
        terminals_[as_uindex(value)] = this->make_new_node(value);
    }

    return id_set_marked(terminals_[as_uindex(value)]);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::make_special_node(
    [[maybe_unused]] int32 const value
) -> node_t*
{
    assert(value == Undefined);

    if (specials_.empty())
    {
        specials_.resize(1, nullptr);
    }

    if (not specials_[0])
    {
        specials_[0] = this->make_new_node(Undefined);
    }

    return id_set_marked(specials_[0]);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::make_internal_node(
    int32 const index,
    son_container&& sons
) -> node_t*
{
    // Each node comming out of here is marked.
    // Later on it must become son of someone or root of a diagram.
    auto ret = static_cast<node_t*>(nullptr);

    if (this->is_redundant(index, sons))
    {
        ret = sons[0];
    }
    else
    {
        auto& table                 = uniqueTables_[as_uindex(index)];
        auto const [existing, hash] = table.find(sons, domains_[index]);
        if (existing)
        {
            ret = existing;
        }
        else
        {
            ret = this->make_new_node(index, std::move(sons));
            table.insert(ret, hash);
            this->for_each_son(ret, id_inc_ref_count<Data, Degree>);
        }

        // It is now safe to unmark them since they certainly have ref.
        this->for_each_son(ret, id_set_notmarked<Data, Degree>);
    }

    return id_set_marked(ret);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_level(int32 const index) const
    -> int32
{
    return indexToLevel_[as_uindex(index)];
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_level(node_t* const node) const
    -> int32
{
    return node->is_terminal() ? this->get_leaf_level()
                               : this->get_level(node->get_index());
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_leaf_level() const -> int32
{
    return static_cast<int32>(this->get_var_count());
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_index(int32 const level) const
    -> int32
{
    assert(level < ssize(levelToIndex_));
    return levelToIndex_[as_uindex(level)];
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_domain(int32 const index) const
    -> int32
{
    assert(index < this->get_var_count());
    return domains_[index];
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_node_count(int32 const index) const
    -> int64
{
    assert(index < this->get_var_count());
    return uniqueTables_[as_uindex(index)].size();
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_node_count(node_t* const node
) const -> int64
{
    auto count = int64(0);
    this->traverse_pre(
        node,
        [&count] (auto const)
        {
            ++count;
        }
    );
    return count;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_node_count() const -> int64
{
    return nodeCount_;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_var_count() const -> int32
{
    return static_cast<int32>(uniqueTables_.size());
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_order() const
    -> std::vector<int32> const&
{
    return levelToIndex_;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_domains() const
    -> std::vector<int32>
{
    auto domains = std::vector<int32>();
    for (auto k = 0; k < this->get_var_count(); ++k)
    {
        domains.emplace_back(domains_[k]);
    }
    return domains;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::force_gc() -> void
{
    this->collect_garbage();
    opCache_.remove_unused();
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::collect_garbage() -> void
{
    debug::out("node_manager: Collecting garbage. ");
    [[maybe_unused]] auto const before = nodeCount_;

    for (auto level = 0; level < this->get_var_count(); ++level)
    {
        auto const index = levelToIndex_[as_uindex(level)];
        auto& table      = uniqueTables_[as_uindex(index)];
        auto const endIt = std::end(table);
        auto tableIt     = std::begin(table);

        while (tableIt != endIt)
        {
            auto const node = *tableIt;
            if (can_be_gced(node))
            {
                this->for_each_son(node, dec_ref_count);
                tableIt = table.erase(tableIt);
                this->delete_node(node);
            }
            else
            {
                ++tableIt;
            }
        }
    }

    for (auto& node : terminals_)
    {
        if (node && can_be_gced(node))
        {
            this->delete_node(node);
            node = nullptr;
        }
    }

    for (auto& node : specials_)
    {
        if (node && can_be_gced(node))
        {
            this->delete_node(node);
            node = nullptr;
        }
    }

    debug::out(
        before - nodeCount_,
        " nodes collected.",
        " Now there are ",
        nodeCount_,
        " unique nodes.\n"
    );
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::to_dot_graph(std::ostream& ost) const
    -> void
{
    this->to_dot_graph_common(
        ost,
        [this] (auto const& operation)
        {
            this->for_each_node(operation);
        }
    );
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::to_dot_graph(
    std::ostream& ost,
    node_t* const node
) const -> void
{
    this->to_dot_graph_common(
        ost,
        [this, node] (auto const& operation)
        {
            this->traverse_pre(node, operation);
        }
    );
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::domain_product(
    int32 const levelFrom,
    int32 const levelTo
) const -> int64
{
    if constexpr (domains::is_fixed<Domain>()())
    {
        return utils::int_pow(Domain()(), levelTo - levelFrom);
    }
    else
    {
        auto product = int64(1);
        for (auto level = levelFrom; level < levelTo; ++level)
        {
            product *= domains_[levelToIndex_[as_uindex(level)]];
        }
        return product;
    }
}

template<class Data, degree Degree, domain Domain>
template<utils::i_gen F>
auto node_manager<Data, Degree, Domain>::make_sons(
    int32 const index,
    F&& generator
) -> son_container
{
    auto sons = node_t::make_son_container(domains_[index], Degree());
    for (auto k = 0; k < domains_[index]; ++k)
    {
        sons[as_uindex(k)] = std::invoke(generator, k);
    }
    return sons;
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::for_each_son(
    node_t* const node,
    NodeOp&& operation
) const -> void
{
    auto const index = node->get_index();
    for (auto k = 0; k < domains_[index]; ++k)
    {
        std::invoke(operation, node->get_son(k));
    }
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::for_each_son(
    int32 const index,
    son_container const& sons,
    NodeOp&& operation
) const -> void
{
    for (auto k = 0; k < domains_[index]; ++k)
    {
        std::invoke(operation, sons[as_uindex(k)]);
    }
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::for_each_node(NodeOp&& operation) const
    -> void
{
    for (auto const& table : uniqueTables_)
    {
        for (auto const node : table)
        {
            std::invoke(operation, node);
        }
    }

    this->for_each_terminal_node(operation);
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::for_each_terminal_node(
    NodeOp&& operation
) const -> void
{
    for (auto const node : terminals_)
    {
        if (node)
        {
            std::invoke(operation, node);
        }
    }
}

template<class Data, degree Degree, domain Domain>
template<teddy_bin_op O>
auto node_manager<Data, Degree, Domain>::cache_find(
    node_t* const lhs,
    node_t* const rhs
) -> node_t*
{
    auto const node = opCache_.find(O::get_id(), lhs, rhs);
    if (node)
    {
        id_set_marked(node);
    }
    return node;
}

template<class Data, degree Degree, domain Domain>
template<teddy_bin_op O>
auto node_manager<Data, Degree, Domain>::cache_put(
    node_t* const result,
    node_t* const lhs,
    node_t* const rhs
) -> void
{
    opCache_.put(O::get_id(), result, lhs, rhs);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::is_valid_var_value(
    int32 const index,
    int32 const value
) const -> bool
{
    return value < domains_[index];
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::run_deferred() -> void
{
    if (gcReorderDeferred_)
    {
        this->collect_garbage();
        opCache_.clear();
        this->sift_variables();
    }
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::traverse_pre(
    node_t* const rootNode,
    NodeOp&& nodeOp
) const -> void
{
    auto const step
        = [this] (auto const& self, node_t* const node, auto const& operation)
        -> void
    {
        node->toggle_marked();
        std::invoke(operation, node);
        if (node->is_internal())
        {
            this->for_each_son(
                node,
                [&self, node, &operation] (auto const son) -> void
                {
                    if (node->is_marked() != son->is_marked())
                    {
                        self(self, son, operation);
                    }
                }
            );
        }
    };

    step(step, rootNode, nodeOp);
    // Second traverse to reset marks.
    this->traverse_no_op(rootNode);
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::traverse_post(
    node_t* const rootNode,
    NodeOp&& nodeOp
) const -> void
{
    auto const step
        = [this] (auto const& self, node_t* const node, auto& operation) -> void
    {
        node->toggle_marked();
        if (node->is_internal())
        {
            this->for_each_son(
                node,
                [&self, node, &operation] (auto const son)
                {
                    if (node->is_marked() != son->is_marked())
                    {
                        self(self, son, operation);
                    }
                }
            );
        }
        std::invoke(operation, node);
    };

    step(step, rootNode, nodeOp);
    // Second traverse to reset marks.
    this->traverse_no_op(rootNode);
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::traverse_level(
    node_t* const rootNode,
    NodeOp&& nodeOp
) const -> void
{
    auto const cmp_levels = [this] (node_t* const lhs, node_t* const rhs)
    {
        return this->get_level(lhs) > this->get_level(rhs);
    };

    using compare_t = decltype(cmp_levels);
    using node_prio_q
        = std::priority_queue<node_t*, std::vector<node_t*>, compare_t>;
    auto queue = node_prio_q(cmp_levels);
    rootNode->toggle_marked();
    queue.push(rootNode);
    while (not queue.empty())
    {
        auto const current = queue.top();
        queue.pop();
        std::invoke(nodeOp, current);
        if (current->is_internal())
        {
            this->for_each_son(
                current,
                [&queue, current] (node_t* const son)
                {
                    if (son->is_marked() != current->is_marked())
                    {
                        queue.push(son);
                        son->toggle_marked();
                    }
                }
            );
        }
    }

    // Second traverse to reset marks.
    this->traverse_no_op(rootNode);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::dec_ref_count(node_t* const node)
    -> void
{
    node->dec_ref_count();
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::is_redundant(
    int32 const index,
    son_container const& sons
) const -> bool
{
    for (auto j = 1; j < domains_[index]; ++j)
    {
        if (sons[as_uindex(j - 1)] != sons[as_uindex(j)])
        {
            return false;
        }
    }
    return true;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::adjust_tables() -> void
{
    debug::out(
        "node_manager: Adjusting unique tables.",
        " Node count is ",
        nodeCount_,
        ".\n"
    );

    for (auto i = 0; i < ssize(uniqueTables_); ++i)
    {
        uniqueTables_[as_uindex(i)].adjust_capacity(domains_[i]);
    }
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::adjust_caches() -> void
{
    auto const newSize    = cacheRatio_ * static_cast<double>(nodeCount_);
    auto const newIntSize = static_cast<int64>(newSize);
    opCache_.adjust_capacity(newIntSize);
}

template<class Data, degree Degree, domain Domain>
template<class... Args>
auto node_manager<Data, Degree, Domain>::make_new_node(Args&&... args)
    -> node_t*
{
    if (autoReorderEnabled_)
    {
        // GC + reorder will be done after current
        // high level operations finishes.
        // Until then, just create a new pool.

        if (pool_.get_available_node_count() == 0)
        {
            pool_.grow();
            this->deferr_gc_reorder();
        }
    }
    else
    {
        // Run GC. If not enough nodes are collected,
        // preventively create a new pool.

        if (pool_.get_available_node_count() == 0)
        {
            auto const growThreshold = static_cast<int64>(
                gcRatio_ * static_cast<double>(pool_.get_main_pool_size())
            );

            this->force_gc();

            if (pool_.get_available_node_count() < growThreshold)
            {
                pool_.grow();
            }
        }
    }

    if (nodeCount_ >= nextTableAdjustment_)
    {
        // When the number of nodes doubles,
        // adjust cache and table sizes.
        this->adjust_tables();
        this->adjust_caches();
        nextTableAdjustment_ *= 2;
    }

    ++nodeCount_;
    return pool_.create(std::forward<Args>(args)...);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::delete_node(node_t* const n) -> void
{
    assert(not n->is_marked());
    --nodeCount_;
    n->set_unused();
    pool_.destroy(n);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::traverse_no_op(node_t* const rootNode
) const -> void
{
    auto const step = [this] (auto const self, node_t* const node) -> void
    {
        node->toggle_marked();
        if (node->is_internal())
        {
            this->for_each_son(
                node,
                [self, node] (auto const son) -> void
                {
                    if (node->is_marked() != son->is_marked())
                    {
                        self(self, son);
                    }
                }
            );
        }
    };
    step(step, rootNode);
}

template<class Data, degree Degree, domain Domain>
template<class ForEachNode>
auto node_manager<Data, Degree, Domain>::to_dot_graph_common(
    std::ostream& ost,
    ForEachNode&& forEach
) const -> void
{
    auto const make_label = [] (auto const n)
    {
        if (n->is_terminal())
        {
            using namespace std::literals::string_literals;
            auto const val = n->get_value();
            return val == Undefined ? "*"s : std::to_string(val);
        }

        return "x" + std::to_string(n->get_index());
    };

    auto const get_id_str = [] (node_t* const n)
    {
        return std::to_string(reinterpret_cast<std::uintmax_t>(n));
    };

    auto const output_range = [] (auto& ostr, auto const& range, auto const sep)
    {
        auto const endIt = end(range);
        auto rangeIt     = begin(range);
        while (rangeIt != endIt)
        {
            ostr << *rangeIt;
            ++rangeIt;
            if (rangeIt != endIt)
            {
                ostr << sep;
            }
        }
    };

    auto const levelCount = as_usize(1 + this->get_var_count());
    auto labels           = std::vector<std::string>();
    auto rankGroups       = std::vector<std::vector<std::string>>(levelCount);
    auto arcs             = std::vector<std::string>();
    auto squareShapes     = std::vector<std::string>();

    forEach(
        [&, this] (auto const n)
        {
            // Create label.
            auto const level = this->get_level(n);
            labels.emplace_back(
                get_id_str(n) + R"( [label = ")" + make_label(n)
                + R"(", tooltip = ")" + std::to_string(n->get_ref_count())
                + R"("];)"
            );

            if (n->is_terminal())
            {
                squareShapes.emplace_back(get_id_str(n));
                rankGroups.back().emplace_back(get_id_str(n) + ";");
                return;
            }

            // Add to same level.
            rankGroups[as_uindex(level)].emplace_back(get_id_str(n) + ";");

            // Add arcs.
            this->for_each_son(
                n,
                [&, sonOrder = 0] (auto const son) mutable
                {
                    if constexpr (std::is_same_v<Degree, degrees::fixed<2>>)
                    {
                        arcs.emplace_back(
                            get_id_str(n) + " -> " + get_id_str(son)
                            + " [style = "
                            + (0 == sonOrder ? "dashed" : "solid") + "];"
                        );
                    }
                    else
                    {
                        arcs.emplace_back(
                            get_id_str(n) + " -> " + get_id_str(son)
                            + R"( [label = )" + std::to_string(sonOrder) + "];"
                        );
                    }
                    ++sonOrder;
                }
            );
        }
    );

    // Finally, output everything into the output stream.
    ost << "digraph DD {" << '\n';
    ost << "    node [shape = square] ";
    output_range(ost, squareShapes, " ");
    ost << ";\n";
    ost << "    node [shape = circle];"
        << "\n\n";

    ost << "    ";
    output_range(ost, labels, "\n    ");
    ost << "\n\n";
    ost << "    ";
    output_range(ost, arcs, "\n    ");
    ost << "\n\n";

    for (auto const& ranks : rankGroups)
    {
        if (not ranks.empty())
        {
            ost << "    { rank = same; ";
            output_range(ost, ranks, " ");
            ost << " }" << '\n';
        }
    }
    ost << '\n';
    ost << "}" << '\n';
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::deferr_gc_reorder() -> void
{
    gcReorderDeferred_ = true;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::check_distinct(
    std::vector<int32> const& ints
) -> bool
{
    if (ints.empty())
    {
        return true;
    }
    auto const maxElem = *std::max_element(std::begin(ints), std::end(ints));
    auto bitset        = std::vector<bool>(as_usize(maxElem + 1), false);
    for (auto const checkInt : ints)
    {
        if (bitset[as_uindex(checkInt)])
        {
            return false;
        }
        bitset[as_uindex(checkInt)] = true;
    }
    return true;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::can_be_gced(node_t* const node) -> bool
{
    return node->get_ref_count() == 0 && not node->is_marked();
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::swap_node_with_next(node_t* const node)
    -> void
{
    auto const mkmatrix = [] (
        [[maybe_unused]] auto const nRow,
        [[maybe_unused]] auto const nCol
    )
    {
        if constexpr (degrees::is_fixed<Degree>()())
        {
            auto constexpr Deg = Degree()();
            return std::array<std::array<node_t*, Deg>, Deg> {};
        }
        else
        {
            auto const row = std::vector<node_t*>(as_usize(nCol), nullptr);
            return std::vector<std::vector<node_t*>>(as_usize(nRow), row);
        }
    };

    auto const nodeIndex  = node->get_index();
    auto const nextIndex  = this->get_index(1 + this->get_level(node));
    auto const nodeDomain = this->get_domain(nodeIndex);
    auto const sonDomain  = this->get_domain(nextIndex);
    auto const oldSons    = this->make_sons(
        nodeIndex,
        [node] (auto const sonOrder)
        {
            return node->get_son(sonOrder);
        }
    );

    auto cofactorMatrix = mkmatrix(nodeDomain, sonDomain);
    for (auto nk = 0; nk < nodeDomain; ++nk)
    {
        auto const son = node->get_son(nk);
        for (auto sk = 0; sk < sonDomain; ++sk)
        {
            auto const justUseSon
                = son->is_terminal() || son->get_index() != nextIndex;
            cofactorMatrix[as_uindex(nk)][as_uindex(sk)]
                = justUseSon ? son : son->get_son(sk);
        }
    }

    node->set_index(nextIndex);
    auto newSons = this->make_sons(
        nextIndex,
        [&, this] (int32 const outerOrder)
        {
            return this->make_internal_node(
                nodeIndex,
                this->make_sons(
                    nodeIndex,
                    [&] (int32 const innerOrder)
                    {
                        return cofactorMatrix[as_uindex(innerOrder)]
                                             [as_uindex(outerOrder)];
                    }
                )
            );
        }
    );
    node->set_sons(std::move(newSons));
    this->for_each_son(node, id_inc_ref_count<Data, Degree>);
    this->for_each_son(node, id_set_notmarked<Data, Degree>);
    this->for_each_son(
        nodeIndex,
        oldSons,
        [this] (auto const oldSon)
        {
            this->dec_ref_try_gc(oldSon);
        }
    );
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::dec_ref_try_gc(node_t* const node)
    -> void
{
    node->dec_ref_count();

    if (not can_be_gced(node))
    {
        return;
    }

    if (node->is_internal())
    {
        this->for_each_son(
            node,
            [this] (node_t* const son)
            {
                this->dec_ref_try_gc(son);
            }
        );

        uniqueTables_[as_uindex(node->get_index())].erase(
            node,
            domains_[node->get_index()]
        );
    }
    else
    {
        if (is_special(node->get_value()))
        {
            specials_[as_uindex(special_to_index(node->get_value()))] = nullptr;
        }
        else
        {
            terminals_[as_uindex(node->get_value())] = nullptr;
        }
    }

    this->delete_node(node);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::swap_variable_with_next(
    int32 const index
) -> void
{
    auto const level     = this->get_level(index);
    auto const nextIndex = this->get_index(1 + level);
    auto tmpTable
        = unique_table<Data, Degree>(std::move(uniqueTables_[as_uindex(index)])
        );
    for (auto const node : tmpTable)
    {
        this->swap_node_with_next(node);
    }
    uniqueTables_[as_uindex(index)].adjust_capacity(domains_[index]);
    uniqueTables_[as_uindex(nextIndex)].merge(
        std::move(tmpTable),
        domains_[nextIndex]
    );

    using std::swap;
    swap(levelToIndex_[as_uindex(level)], levelToIndex_[as_uindex(1 + level)]);
    ++indexToLevel_[as_uindex(index)];
    --indexToLevel_[as_uindex(nextIndex)];
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::sift_variables() -> void
{
    using count_pair = struct
    {
        int32 index_;
        int64 count_;
    };

    // Sorts indices by number of nodes with given index descending.
    auto const determine_sift_order = [this] ()
    {
        auto const varCount = this->get_var_count();
        auto counts         = utils::fill_vector(
            varCount,
            [this] (int32 const index)
            {
                return count_pair {index, this->get_node_count(index)};
            }
        );
        std::sort(
            std::begin(counts),
            std::end(counts),
            [] (auto const& lhs, auto const& rhs)
            {
                return lhs.count_ > rhs.count_;
            }
        );
        return counts;
    };

    // Moves variable one level down.
    auto const move_var_down = [this] (auto const index)
    {
        this->swap_variable_with_next(index);
    };

    // Moves variable one level up.
    auto const move_var_up = [this] (auto const index)
    {
        auto const level     = this->get_level(index);
        auto const prevIndex = this->get_index(level - 1);
        this->swap_variable_with_next(prevIndex);
    };

    // Tries to place variable on each level.
    // In the end, restores position with lowest total number of nodes.
    auto const place_variable = [&, this] (auto const index)
    {
        auto const lastInternalLevel = this->get_var_count() - 1;
        auto currentLevel            = this->get_level(index);
        auto optimalLevel            = currentLevel;
        auto optimalCount            = nodeCount_;

        // Sift down.
        while (currentLevel != lastInternalLevel)
        {
            move_var_down(index);
            ++currentLevel;
            if (nodeCount_ < optimalCount)
            {
                optimalCount = nodeCount_;
                optimalLevel = currentLevel;
            }
        }

        // Sift up.
        while (currentLevel != 0)
        {
            move_var_up(index);
            --currentLevel;
            if (nodeCount_ < optimalCount)
            {
                optimalCount = nodeCount_;
                optimalLevel = currentLevel;
            }
        }

        // Restore optimal position.
        while (currentLevel != optimalLevel)
        {
            move_var_down(index);
            ++currentLevel;
        }
    };

    debug::out(
        "node_manager: Sifting variables. Node count before ",
        nodeCount_,
        ".\n"
    );

    auto const siftOrder = determine_sift_order();
    for (auto const pair : siftOrder)
    {
        place_variable(pair.index_);
    }

    debug::out(
        "node_manager: Done sifting. Node count after ",
        nodeCount_,
        ".\n"
    );

    gcReorderDeferred_ = false;
}
} // namespace teddy

#endif