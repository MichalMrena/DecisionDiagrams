#ifndef LIBTEDDY_DETAILS_NODE_MANAGER_HPP
#define LIBTEDDY_DETAILS_NODE_MANAGER_HPP

#include <libteddy/details/config.hpp>
#include <libteddy/details/debug.hpp>
#include <libteddy/details/hash_tables.hpp>
#include <libteddy/details/node.hpp>
#include <libteddy/details/node_pool.hpp>
#include <libteddy/details/operators.hpp>
#include <libteddy/details/tools.hpp>
#include <libteddy/details/types.hpp>

#include <cassert>
#include <concepts>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace teddy
{
namespace domains
{
struct mixed
{
    std::vector<int32> domains_;

    mixed(std::vector<int32> domains) :
        domains_(static_cast<std::vector<int32>&&>(domains))
    {
    };

    auto operator[] (int32 const index) const
    {
        return domains_[as_uindex(index)];
    }
};

template<int32 N>
struct fixed
{
    static_assert(N > 1);

    static constexpr int32 value = N;

    constexpr auto operator[] ([[maybe_unused]] int32 const index) const
    {
        return N;
    }
};

template<class T>
struct is_fixed
{
    static constexpr bool value = false;
};

template<int32 N>
struct is_fixed<fixed<N>>
{
    static constexpr bool value = true;
};

template<class T>
struct is_mixed
{
    static constexpr bool value = false;
};

template<>
struct is_mixed<mixed>
{
    static constexpr bool value = true;
};
} // namespace domains

template<class Data, class Degree, class Domain>
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
    requires(domains::is_fixed<Domain>::value);

    node_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        std::vector<int32> order,
        domains::mixed domains
    )
    requires(domains::is_mixed<Domain>::value);

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
    [[nodiscard]] auto make_internal_node (
        int32 index,
        son_container const& sons
    ) -> node_t*;
    [[nodiscard]] auto make_son_container (int32 domain) -> son_container;
    [[nodiscard]] auto get_level (int32 index) const -> int32;
    [[nodiscard]] auto get_level (node_t* node) const -> int32;
    [[nodiscard]] auto get_leaf_level () const -> int32;
    [[nodiscard]] auto get_index (int32 level) const -> int32;
    [[nodiscard]] auto get_domain (int32 index) const -> int32;
    [[nodiscard]] auto get_domain (node_t* node) const -> int32;
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

    template<class NodeOp>
    auto for_each_son (node_t* node, NodeOp&& operation) const -> void;

    template<class NodeOp>
    auto for_each_son (
        int32 index,
        son_container const& sons,
        NodeOp&& operation
    ) const -> void;

    template<class NodeOp>
    auto for_each_node (NodeOp&& operation) const -> void;

    template<class NodeOp>
    auto for_each_terminal_node (NodeOp&& operation) const -> void;

    template<teddy_bin_op O>
    [[nodiscard]] auto cache_find (node_t* lhs, node_t* rhs) -> node_t*;

    template<teddy_bin_op O>
    auto cache_put (node_t* result, node_t* lhs, node_t* rhs) -> void;

    auto cache_clear () -> void;

    template<class NodeOp>
    auto traverse_pre (node_t* rootNode, NodeOp operation) const -> void;

    template<class NodeOp>
    auto traverse_post (node_t* rootNode, NodeOp operation) const -> void;

    template<class NodeOp>
    auto traverse_level (node_t* rootNode, NodeOp operation) const -> void;

    [[nodiscard]] auto is_valid_var_value (int32 index, int32 value) const
        -> bool;

    auto run_deferred () -> void;

    static auto dec_ref_count (node_t* node) -> void;

    auto sift_variables () -> void;

private:
    template<class NodeOp>
    auto traverse_pre_impl (node_t* node, NodeOp operation) const -> void;

    template<class NodeOp>
    auto traverse_post_impl (node_t* node, NodeOp operation) const -> void;

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

    template<class ForEachNode>
    auto to_dot_graph_common (std::ostream& ost, ForEachNode&& forEach) const
        -> void;

    auto deferr_gc_reorder () -> void;

    auto collect_garbage () -> void;

    [[nodiscard]] static auto check_distinct (std::vector<int32> const& ints)
        -> bool;

    [[nodiscard]] static auto can_be_gced (node_t* node) -> bool;

private:
    static constexpr int32 DEFAULT_FIRST_TABLE_ADJUSTMENT = 230;
    static constexpr double DEFAULT_CACHE_RATIO = 1.0;
    static constexpr double DEFAULT_GC_RATIO = 0.20;

private:
    apply_cache<Data, Degree> opCache_;
    node_pool<Data, Degree> pool_;
    std::vector<unique_table<Data, Degree>> uniqueTables_;
    std::vector<node_t*> terminals_;
    std::vector<node_t*> specials_;
    std::vector<int32> indexToLevel_;
    std::vector<int32> levelToIndex_;
    [[no_unique_address]] Domain domains_;
    int32 varCount_;
    int64 nodeCount_;
    int64 adjustmentNodeCount_;
    double cacheRatio_;
    double gcRatio_;
    bool autoReorderEnabled_;
    bool gcReorderDeferred_;
};

template<class Data, class Degree>
auto id_inc_ref_count (node<Data, Degree>* const node) -> ::teddy::node<Data, Degree>*
{
    node->inc_ref_count();
    return node;
}

template<class Data, class Degree>
auto id_set_marked (node<Data, Degree>* const node) -> ::teddy::node<Data, Degree>*
{
    node->set_marked();
    return node;
}

template<class Data, class Degree>
auto id_set_notmarked (node<Data, Degree>* const node) -> ::teddy::node<Data, Degree>*
{
    node->set_notmarked();
    return node;
}

template<class Data, class Degree, class Domain>
node_manager<Data, Degree, Domain>::node_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> order
)
requires(domains::is_fixed<Domain>::value)
    :
    node_manager(
        common_init_tag(),
        varCount,
        nodePoolSize,
        overflowNodePoolSize,
        static_cast<std::vector<int32>&&>(order),
        {}
    )
{
}

template<class Data, class Degree, class Domain>
node_manager<Data, Degree, Domain>::node_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> order,
    domains::mixed domains
)
requires(domains::is_mixed<Domain>::value)
    :
    node_manager(
        common_init_tag(),
        varCount,
        nodePoolSize,
        overflowNodePoolSize,
        static_cast<std::vector<int32>&&>(order),
        static_cast<domains::mixed&&>(domains)
    )
{
}

template<class Data, class Degree, class Domain>
node_manager<Data, Degree, Domain>::node_manager(
    [[maybe_unused]] common_init_tag initTag,
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> order,
    Domain domains
) :
    opCache_(static_cast<int64>(
        DEFAULT_CACHE_RATIO * static_cast<double>(nodePoolSize))
    ),
    pool_(nodePoolSize, overflowNodePoolSize),
    uniqueTables_(),
    terminals_(),
    specials_(),
    indexToLevel_(as_usize(varCount)),
    levelToIndex_(static_cast<std::vector<int32>&&>(order)),
    domains_(static_cast<Domain&&>(domains)),
    varCount_(varCount),
    nodeCount_(0),
    adjustmentNodeCount_(DEFAULT_FIRST_TABLE_ADJUSTMENT),
    cacheRatio_(DEFAULT_CACHE_RATIO),
    gcRatio_(DEFAULT_GC_RATIO),
    autoReorderEnabled_(false),
    gcReorderDeferred_(false)
{
    assert(ssize(levelToIndex_) == varCount_);
    assert(check_distinct(levelToIndex_));

    if constexpr (domains::is_mixed<Domain>::value)
    {
        assert(ssize(domains_.domains_) == varCount_);
        if constexpr (degrees::is_fixed<Degree>::value)
        {
            for ([[maybe_unused]] int32 const domain : domains_.domains_)
            {
                assert(domain <= Degree::value);
            }
        }
    }

    // Create reverse mapping from (level -> index) to (index -> level)
    int32 level = 0;
    for (int32 const index : levelToIndex_)
    {
        indexToLevel_[as_uindex(index)] = level++;
    }

    // Initialize unique tables with pre-allocated sizes
    // The sizes follow triangular distribution with
    // a = 0
    // c = RelPeakPosition * varCount
    // b = varCount
    // f(c) = RelPeakNodeCount * nodePoolSize

    // These two magic numbers are empirically obtained
    double constexpr RelPeakPosition  = 0.71;
    double constexpr RelPeakNodeCount = 0.05;
    double const c  = RelPeakPosition * (static_cast<double>(varCount) - 1);
    double const fc = RelPeakNodeCount * static_cast<double>(nodePoolSize);

    for (int32 i = 0; i <= static_cast<int32>(c); ++i)
    {
        auto const x = static_cast<double>(i);
        uniqueTables_.emplace_back(static_cast<int64>(fc * x / c));
    }

    for (auto i = static_cast<int32>(c) + 1; i < varCount_; ++i)
    {
        auto const n = static_cast<double>(varCount) - 1;
        auto const x = static_cast<double>(i);
        uniqueTables_.emplace_back(
            static_cast<int64>((fc * x) / (c - n) - (fc * n) / (c - n))
        );
    }
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::set_cache_ratio(double const ratio)
    -> void
{
    assert(ratio > 0);
    cacheRatio_ = ratio;
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::set_gc_ratio(double const ratio)
    -> void
{
    assert(ratio >= 0.0 && ratio <= 1.0);
    gcRatio_ = ratio;
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::set_auto_reorder(bool const doReorder)
    -> void
{
    autoReorderEnabled_ = doReorder;
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_terminal_node(int32 const value
) const -> node_t*
{
    return value < ssize(terminals_) ? terminals_[as_uindex(value)] : nullptr;
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::make_terminal_node(int32 const value)
    -> node_t*
{
    if constexpr (domains::is_fixed<Domain>::value)
    {
        assert(value < Domain::value);
    }

    if (is_special(value))
    {
        return this->make_special_node(value);
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

template<class Data, class Degree, class Domain>
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

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::make_son_container
    (int32 const domain) -> son_container
{
    return node_t::make_son_container(domain, Degree());
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::make_internal_node(
    int32 const index,
    son_container const& sons
) -> node_t*
{
    // Each node comming out of here is marked.
    // Later on it must become son of someone or root of a diagram.
    auto ret = static_cast<node_t*>(nullptr);

    if (this->is_redundant(index, sons))
    {
        ret = sons[0];
        if constexpr (degrees::is_mixed<Degree>::value)
        {
            node_t::delete_son_container(sons);
        }
    }
    else
    {
        auto& table                 = uniqueTables_[as_uindex(index)];
        auto const [existing, hash] = table.find(sons, domains_[index]);
        if (existing)
        {
            ret = existing;
            if constexpr (degrees::is_mixed<Degree>::value)
            {
                node_t::delete_son_container(sons);
            }
        }
        else
        {
            ret = this->make_new_node(index, sons);
            table.insert(ret, hash);
            this->for_each_son(ret, id_inc_ref_count<Data, Degree>);
        }

        // It is now safe to unmark them since they certainly have ref.
        this->for_each_son(ret, id_set_notmarked<Data, Degree>);
    }

    return id_set_marked(ret);
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_level(int32 const index) const
    -> int32
{
    return indexToLevel_[as_uindex(index)];
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_level(node_t* const node) const
    -> int32
{
    return node->is_terminal() ? this->get_leaf_level()
                               : this->get_level(node->get_index());
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_leaf_level() const -> int32
{
    return static_cast<int32>(this->get_var_count());
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_index(int32 const level) const
    -> int32
{
    assert(level < ssize(levelToIndex_));
    return levelToIndex_[as_uindex(level)];
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_domain(int32 const index) const
    -> int32
{
    assert(index < this->get_var_count());
    return domains_[index];
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_domain(node_t* const node) const
    -> int32
{
    return this->get_domain(node->get_index());
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_node_count(int32 const index) const
    -> int64
{
    assert(index < this->get_var_count());
    return uniqueTables_[as_uindex(index)].size();
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_node_count(node_t* const node
) const -> int64
{
    int64 count = 0;
    this->traverse_pre(
        node,
        [&count] (node_t*)
        {
            ++count;
        }
    );
    return count;
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_node_count() const -> int64
{
    return nodeCount_;
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_var_count() const -> int32
{
    return varCount_;
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_order() const
    -> std::vector<int32> const&
{
    return levelToIndex_;
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::get_domains() const
    -> std::vector<int32>
{
    std::vector<int32> domains;
    domains.reserve(as_usize(varCount_));
    for (int32 k = 0; k < varCount_; ++k)
    {
        domains.push_back(domains_[k]);
    }
    return domains;
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::force_gc() -> void
{
    this->collect_garbage();
    opCache_.remove_unused();
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::collect_garbage() -> void
{
    #ifdef LIBTEDDY_VERBOSE
    debug::out("node_manager: Collecting garbage. ");
    int64 const before = nodeCount_;
    #endif

    for (int32 level = 0; level < this->get_var_count(); ++level)
    {
        int32 const index = levelToIndex_[as_uindex(level)];
        auto& table      = uniqueTables_[as_uindex(index)];
        auto const endIt = table.end();
        auto tableIt     = table.begin();

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

    // TODO make terminals live forever
    for (node_t* const node : terminals_)
    {
        if (node && can_be_gced(node))
        {
            this->delete_node(node);
            node = nullptr;
        }
    }

    for (node_t* const node : specials_)
    {
        if (node && can_be_gced(node))
        {
            this->delete_node(node);
            node = nullptr;
        }
    }

    #ifdef LIBTEDDY_VERBOSE
    debug::out(
        before - nodeCount_,
        " nodes collected.",
        " Now there are ",
        nodeCount_,
        " unique nodes.\n"
    );
    #endif
}

template<class Data, class Degree, class Domain>
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

template<class Data, class Degree, class Domain>
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

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::domain_product(
    int32 const levelFrom,
    int32 const levelTo
) const -> int64
{
    if constexpr (domains::is_fixed<Domain>::value)
    {
        return utils::int_pow(Domain::value, levelTo - levelFrom);
    }
    else
    {
        int64 product = 1;
        for (int32 level = levelFrom; level < levelTo; ++level)
        {
            product *= domains_[levelToIndex_[as_uindex(level)]];
        }
        return product;
    }
}

template<class Data, class Degree, class Domain>
template<class NodeOp>
auto node_manager<Data, Degree, Domain>::for_each_son(
    node_t* const node,
    NodeOp&& operation
) const -> void
{
    int32 const index = node->get_index();
    for (int32 k = 0; k < domains_[index]; ++k)
    {
        operation(node->get_son(k));
    }
}

template<class Data, class Degree, class Domain>
template<class NodeOp>
auto node_manager<Data, Degree, Domain>::for_each_son(
    int32 const index,
    son_container const& sons,
    NodeOp&& operation
) const -> void
{
    for (auto k = 0; k < domains_[index]; ++k)
    {
        operation(sons[as_uindex(k)]);
    }
}

template<class Data, class Degree, class Domain>
template<class NodeOp>
auto node_manager<Data, Degree, Domain>::for_each_node(NodeOp&& operation) const
    -> void
{
    for (auto const& table : uniqueTables_)
    {
        for (node_t* const node : table)
        {
            operation(node);
        }
    }

    this->for_each_terminal_node(operation);
}

template<class Data, class Degree, class Domain>
template<class NodeOp>
auto node_manager<Data, Degree, Domain>::for_each_terminal_node(
    NodeOp&& operation
) const -> void
{
    for (node_t* const node : terminals_)
    {
        if (node)
        {
            operation(node);
        }
    }
}

template<class Data, class Degree, class Domain>
template<teddy_bin_op O>
auto node_manager<Data, Degree, Domain>::cache_find(
    node_t* lhs,
    node_t* rhs
) -> node_t*
{
    if constexpr (O::is_commutative())
    {
        if (rhs < lhs)
        {
            utils::swap(lhs, rhs);
        }
    }
    node_t* const node = opCache_.find(O::get_id(), lhs, rhs);
    if (node)
    {
        id_set_marked(node);
    }
    return node;
}

template<class Data, class Degree, class Domain>
template<teddy_bin_op O>
auto node_manager<Data, Degree, Domain>::cache_put(
    node_t* const result,
    node_t* lhs,
    node_t* rhs
) -> void
{
    if constexpr (O::is_commutative())
    {
        if (rhs < lhs)
        {
            utils::swap(lhs, rhs);
        }
    }
    opCache_.put(O::get_id(), result, lhs, rhs);
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::cache_clear () -> void
{
    opCache_.clear();
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::is_valid_var_value(
    int32 const index,
    int32 const value
) const -> bool
{
    return value < domains_[index];
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::run_deferred() -> void
{
    if (gcReorderDeferred_)
    {
        this->collect_garbage();
        opCache_.clear(); // TODO why not remove_unused?
        this->sift_variables();
    }
}

template<class Data, class Degree, class Domain>
template<class NodeOp>
auto node_manager<Data, Degree, Domain>::traverse_pre(
    node_t* const rootNode,
    NodeOp const operation
) const -> void
{
    this->traverse_pre_impl(rootNode, operation);
    this->traverse_pre_impl(rootNode, [](node_t*){});
    // Second traverse to reset marks
}

template<class Data, class Degree, class Domain>
template<class NodeOp>
auto node_manager<Data, Degree, Domain>::traverse_pre_impl(
    node_t* const node,
    NodeOp const operation
) const -> void
{
    node->toggle_marked();
    operation(node);
    if (node->is_internal())
    {
        int32 const nodeDomain = this->get_domain(node);
        for (int32 k = 0; k < nodeDomain; ++k)
        {
            node_t* const son = node->get_son(k);
            if (node->is_marked() != son->is_marked())
            {
                this->traverse_pre_impl(son, operation);
            }
        }
    }
}

template<class Data, class Degree, class Domain>
template<class NodeOp>
auto node_manager<Data, Degree, Domain>::traverse_post(
    node_t* const rootNode,
    NodeOp operation
) const -> void
{
    this->traverse_post_impl(rootNode, operation);
    this->traverse_post_impl(rootNode, [](node_t*){});
    // Second traverse to reset marks.
}

template<class Data, class Degree, class Domain>
template<class NodeOp>
auto node_manager<Data, Degree, Domain>::traverse_post_impl(
    node_t* const node,
    NodeOp operation
) const -> void
{
    node->toggle_marked();
    if (node->is_internal())
    {
        int32 const nodeDomain = this->get_domain(node);
        for (int32 k = 0; k < nodeDomain; ++k)
        {
            node_t* const son = node->get_son(k);
            if (node->is_marked() != son->is_marked())
            {
                this->traverse_post_impl(son, operation);
            }
        }
    }
    operation(node);
}

template<class Data, class Degree, class Domain>
template<class NodeOp>
auto node_manager<Data, Degree, Domain>::traverse_level(
    node_t* const rootNode,
    NodeOp operation
) const -> void
{
    std::vector<std::vector<node_t*>> buckets(as_usize(varCount_) + 1);
    auto const endBucketIt = end(buckets);
    auto bucketIt = begin(buckets) + this->get_level(rootNode);
    (*bucketIt).push_back(rootNode);
    rootNode->toggle_marked();

    while (bucketIt != endBucketIt)
    {
        for (node_t* const node : *bucketIt)
        {
            operation(node);
            if (node->is_internal())
            {
                int32 const domain = this->get_domain(node);
                for (int32 k = 0; k < domain; ++k)
                {
                    node_t* const son = node->get_son(k);
                    if (son->is_marked() != node->is_marked())
                    {
                        int32 const level = this->get_level(son);
                        buckets[as_uindex(level)].push_back(son);
                        son->toggle_marked();
                    }
                }
            }
        }

        do
        {
            ++bucketIt;
        }
        while (bucketIt != endBucketIt && (*bucketIt).empty());

    }

    // Second traverse to reset marks.
    this->traverse_pre_impl(rootNode, [](node_t*){});
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::dec_ref_count(node_t* const node)
    -> void
{
    node->dec_ref_count();
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::is_redundant(
    int32 const index,
    son_container const& sons
) const -> bool
{
    for (int32 j = 1; j < domains_[index]; ++j)
    {
        if (sons[as_uindex(j - 1)] != sons[as_uindex(j)])
        {
            return false;
        }
    }
    return true;
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::adjust_tables() -> void
{
    #ifdef LIBTEDDY_VERBOSE
    debug::out(
        "node_manager: Adjusting unique tables.",
        " Node count is ",
        nodeCount_,
        ".\n"
    );
    #endif

    for (int32 i = 0; i < ssize(uniqueTables_); ++i)
    {
        uniqueTables_[as_uindex(i)].adjust_capacity(domains_[i]);
    }
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::adjust_caches() -> void
{
    auto const newCapacity = cacheRatio_ * static_cast<double>(nodeCount_);
    opCache_.grow_capacity(static_cast<int64>(newCapacity));
}

template<class Data, class Degree, class Domain>
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

    if (nodeCount_ >= adjustmentNodeCount_)
    {
        // When the number of nodes doubles,
        // adjust cache and table sizes.
        this->adjust_tables();
        this->adjust_caches();
        adjustmentNodeCount_ *= 2;
    }

    ++nodeCount_;
    return pool_.create(args...);
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::delete_node(node_t* const n) -> void
{
    assert(not n->is_marked());
    --nodeCount_;
    n->set_unused();
    pool_.destroy(n);
}

template<class Data, class Degree, class Domain>
template<class ForEachNode>
auto node_manager<Data, Degree, Domain>::to_dot_graph_common(
    std::ostream& ost,
    ForEachNode&& forEach
) const -> void
{
    auto const make_label = [] (node_t* const node)
    {
        if (node->is_terminal())
        {
            using namespace std::literals::string_literals;
            int32 const val = node->get_value();
            return val == Undefined ? "*"s : std::to_string(val);
        }

        return "x" + std::to_string(node->get_index());
    };

    auto const get_id_str = [] (node_t* const n)
    {
        return std::to_string(reinterpret_cast<uint64>(n));
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
    std::vector<std::string> labels;
    std::vector<std::vector<std::string>> rankGroups(levelCount);
    std::vector<std::string> arcs;
    std::vector<std::string> squareShapes;

    forEach(
        [&, this] (node_t* const node)
        {
            // Create label.
            int32 const level = this->get_level(node);
            labels.emplace_back(
                get_id_str(node) + R"( [label = ")" + make_label(node)
                + R"(", tooltip = ")" + std::to_string(node->get_ref_count())
                + R"("];)"
            );

            if (node->is_terminal())
            {
                squareShapes.emplace_back(get_id_str(node));
                rankGroups.back().emplace_back(get_id_str(node) + ";");
                return;
            }

            // Add to same level.
            rankGroups[as_uindex(level)].emplace_back(get_id_str(node) + ";");

            // Add arcs.
            this->for_each_son(
                node,
                [&, sonOrder = 0] (node_t* const son) mutable
                {
                    if constexpr (std::is_same_v<Degree, degrees::fixed<2>>)
                    {
                        arcs.emplace_back(
                            get_id_str(node) + " -> " + get_id_str(son)
                            + " [style = "
                            + (0 == sonOrder ? "dashed" : "solid") + "];"
                        );
                    }
                    else
                    {
                        arcs.emplace_back(
                            get_id_str(node) + " -> " + get_id_str(son)
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

    for (std::vector<std::string> const& ranks : rankGroups)
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

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::deferr_gc_reorder() -> void
{
    gcReorderDeferred_ = true;
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::check_distinct(
    std::vector<int32> const& ints
) -> bool
{
    if (ints.empty())
    {
        return true;
    }

    int32 const maxElem = *utils::max_elem(ints.begin(), ints.end());
    std::vector<bool> bitset(as_usize(maxElem + 1), false);
    for (int32 const checkInt : ints)
    {
        if (bitset[as_uindex(checkInt)])
        {
            return false;
        }
        bitset[as_uindex(checkInt)] = true;
    }
    return true;
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::can_be_gced(node_t* const node) -> bool
{
    return node->get_ref_count() == 0 && not node->is_marked();
}

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::swap_node_with_next(node_t* const node)
    -> void
{
    using node_matrix = utils::type_if<
        degrees::is_fixed<Degree>::value,
        node_t*[Degree::value][Degree::value],
        std::vector<std::vector<node_t*>>
    >::type;

    int32 const nodeIndex  = node->get_index();
    int32 const nextIndex  = this->get_index(1 + this->get_level(node));
    int32 const nodeDomain = this->get_domain(nodeIndex);
    int32 const nextDomain = this->get_domain(nextIndex);
    son_container oldSons = this->make_son_container(nodeDomain);
    for (int32 k = 0; k < nodeDomain; ++k)
    {
        oldSons[k] = node->get_son(k);
    }

    node_matrix cofactorMatrix;
    if constexpr (degrees::is_mixed<Degree>::value)
    {
        cofactorMatrix.resize(
            as_usize(nodeDomain),
            std::vector<node_t*>(as_usize(nextDomain))
        );
    }

    for (auto nk = 0; nk < nodeDomain; ++nk)
    {
        node_t* const son = node->get_son(nk);
        for (auto sk = 0; sk < nextDomain; ++sk)
        {
            bool const justUseSon
                = son->is_terminal() || son->get_index() != nextIndex;
            cofactorMatrix[as_uindex(nk)][as_uindex(sk)]
                = justUseSon ? son : son->get_son(sk);
        }
    }

    son_container outerSons = this->make_son_container(nextDomain);
    for (int32 outerK = 0; outerK < nextDomain; ++outerK)
    {
        son_container innerSons = this->make_son_container(nodeDomain);
        for (int32 innerK = 0; innerK < nodeDomain; ++innerK)
        {
            innerSons[innerK] = cofactorMatrix[as_uindex(innerK)][as_uindex(outerK)];
        }
        outerSons[outerK] = this->make_internal_node(nodeIndex, innerSons);
    }

    node->set_index(nextIndex);
    node->set_sons(outerSons);

    for (int32 k = 0; k < nextDomain; ++k)
    {
        node->get_son(k)->inc_ref_count();
        node->get_son(k)->set_notmarked();
    }

    for (int32 k = 0; k < nodeDomain; ++k)
    {
        this->dec_ref_try_gc(oldSons[k]);
    }

    if constexpr (degrees::is_mixed<Degree>::value)
    {
        node_t::delete_son_container(oldSons);
    }
}

template<class Data, class Degree, class Domain>
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
        int32 const nodeDomain = this->get_domain(node);
        for (int32 k = 0; k < nodeDomain; ++k)
        {
            this->dec_ref_try_gc(node->get_son(k));
        }

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

template<class Data, class Degree, class Domain>
auto node_manager<Data, Degree, Domain>::swap_variable_with_next(
    int32 const index
) -> void
{
    auto const level     = this->get_level(index);
    auto const nextIndex = this->get_index(1 + level);
    unique_table<Data, Degree> tmpTable(
        static_cast<unique_table<Data, Degree>&&>(
            uniqueTables_[as_uindex(index)]
        )
    );
    for (auto const node : tmpTable)
    {
        this->swap_node_with_next(node);
    }
    uniqueTables_[as_uindex(index)].adjust_capacity(domains_[index]);
    uniqueTables_[as_uindex(nextIndex)].merge(
        static_cast<unique_table<Data, Degree>&&>(tmpTable),
        domains_[nextIndex]
    );

    utils::swap(
        levelToIndex_[as_uindex(level)],
        levelToIndex_[as_uindex(1 + level)]
    );
    ++indexToLevel_[as_uindex(index)];
    --indexToLevel_[as_uindex(nextIndex)];
}

template<class Data, class Degree, class Domain>
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
        std::vector<count_pair> counts;
        counts.reserve(as_usize(varCount_));
        for (int32 index = 0; index < varCount_; ++index)
        {
            counts.push_back(count_pair {index, this->get_node_count(index)});
        }
        utils::sort(
            counts,
            [] (count_pair const& lhs, count_pair const& rhs)
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

    #ifdef LIBTEDDY_VERBOSE
    debug::out(
        "node_manager: Sifting variables. Node count before ",
        nodeCount_,
        ".\n"
    );
    #endif

    auto const siftOrder = determine_sift_order();
    for (auto const pair : siftOrder)
    {
        place_variable(pair.index_);
    }

    #ifdef LIBTEDDY_VERBOSE
    debug::out(
        "node_manager: Done sifting. Node count after ",
        nodeCount_,
        ".\n"
    );
    #endif

    gcReorderDeferred_ = false;
}
} // namespace teddy

#endif