#ifndef LIBTEDDY_DETAILS_ZDD_MANAGER_HPP
#define LIBTEDDY_DETAILS_ZDD_MANAGER_HPP

#include <libteddy/impl/diagram_manager.hpp>
#include <libteddy/impl/node_manager.hpp>
#include <libteddy/impl/node.hpp>
#include <libteddy/inc/io.hpp>
#include <iostream>
#include <libteddy/impl/operators.hpp>

namespace teddy
{

namespace ops {
    struct ZDD_UNION : details::operation_info<101, true> {};
    struct ZDD_INTERSECT : details::operation_info<102, true> {};
    struct ZDD_DIFFERENCE : details::operation_info<103, false> {};
    struct ZDD_CHANGE : details::operation_info<104, false> {};
    struct ZDD_SUBSET1 : details::operation_info<105, false> {};
    struct ZDD_SUBSET0 : details::operation_info<106, false> {};
    struct ZDD_COUNT : details::operation_info<107, false> {};
} //namespace ops

/**
 * @brief Entry stored in unary operation cache.
 *
 * Represents a cached result of a unary ZDD operation
 * identified by operation ID, input node and optional parameter.
 *
 * The result can be either a node pointer (for ZDD operations)
 * or a numeric value (for count operation).
 */
struct unary_cache_entry {
    #define DOMAIN_SIZE 2
    using node_t = node_manager<degrees::fixed<DOMAIN_SIZE>, domains::fixed<DOMAIN_SIZE>>::node_t;
    int32 opId;
    node_t* node;
    int32 var;        // -1 for count
    node_t* result;   // for subset/change
    int64 number;     // for count
};

class unary_cache {
private:
    #define DOMAIN_SIZE 2
    using node_t = node_manager<degrees::fixed<DOMAIN_SIZE>, domains::fixed<DOMAIN_SIZE>>::node_t;
public:
    explicit unary_cache(int64 capacity)
        : size_(0),
          capacity_(table_base::get_gte_capacity(capacity)),
          entries_(callocate_entries(capacity_)) {}

    ~unary_cache() {
        std::free(entries_);
    }

    unary_cache(unary_cache const&) = delete;
    auto operator=(unary_cache const&) -> unary_cache& = delete;

    unary_cache(unary_cache&&) = delete;
    auto operator=(unary_cache&&) -> unary_cache& = delete;

    /**
    * @brief Looks up a cached result of a unary operation.
    *
    * Searches for a previously computed result identified by
    * operation ID, input node and parameter.
    *
     * @param opId Identifier of the operation.
    * @param node Input node (subgraph root).
    * @param var Additional parameter (or -1 if unused).
     * @return Pointer to cache entry if found, nullptr otherwise.
    */
    auto find(int32 opId, node_t* node, int32 var) -> unary_cache_entry* {
        size_t hash = 0;
        tools::add_hash(hash, opId);
        tools::add_hash(hash, node);
        tools::add_hash(hash, var);

        size_t index = hash % static_cast<size_t>(capacity_);
        auto& e = entries_[index];

        if (e.node != nullptr && e.opId == opId && e.node == node && e.var == var) {
            return &e;
        }
        return nullptr;
    }

    /**
    * @brief Stores result of a unary operation in cache.
    *
    * Inserts or overwrites a cache entry corresponding to the given
    * operation, input node and parameter.
    *
    * @param opId Identifier of the operation.
    * @param node Input node (subgraph root).
    * @param var Additional parameter (or -1 if unused).
    * @param result Result node (nullptr for numeric operations).
    * @param number Numeric result (0 if unused).
    */
    auto put(int32 opId, node_t* node, int32 var,
             node_t* result, int64 number) -> void {
        size_t hash = 0;
        tools::add_hash(hash, opId);
        tools::add_hash(hash, node);
        tools::add_hash(hash, var);

        size_t index = hash % static_cast<size_t>(capacity_);
        auto& e = entries_[index];

        if (e.node != nullptr) {
            ++size_;
        }

        e.opId = opId;
        e.node = node;
        e.var = var;
        e.result = result;
        e.number = number;
    }

    /**
    * @brief Clears all cached entries.
    *
    * Resets the cache to empty state.
    */
    auto clear() -> void {
        size_ = 0;
        std::memset(
            entries_,
            0,
            static_cast<size_t>(capacity_) * sizeof(unary_cache_entry)
    );
}

private:
    static auto callocate_entries(int64 count) -> unary_cache_entry* {
        return static_cast<unary_cache_entry*>(
            std::calloc(static_cast<size_t>(count), sizeof(unary_cache_entry))
        );
    }

    int64 size_;
    int64 capacity_;
    unary_cache_entry* entries_;
};

class zdd_manager
{
private:
    #define DOMAIN_SIZE 2

public:
    using node_t = node_manager<degrees::fixed<DOMAIN_SIZE>, domains::fixed<DOMAIN_SIZE>>::node_t;
    using diagram_t = diagram<degrees::fixed<DOMAIN_SIZE>>;
    
    zdd_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 extraNodePoolSize,
        std::vector<int32> order = {}
    ) :
    m_nodes(
        varCount,
        nodePoolSize,
        extraNodePoolSize,
        detail::default_or_fwd(varCount, TEDDY_MOVE(order))
    ), 
    m_unary_cache(nodePoolSize) {
    }

    /**
    * @brief Constructs a ZDD from a truth table vector.
    *
    * The input vector represents function values for all assignments
    * (its size must be a power of two). The function builds the ZDD
    * bottom-up by creating terminal nodes and iteratively combining them
    * into higher-level nodes using a stack-based reduction.
    *
    * @param vector Truth table values (0/1) in lexicographic order.
    * @return Root node of the constructed ZDD, or nullptr if input is invalid.
    */
    auto from_vector(const std::vector<int>& vector) -> node_t* {
        if (vector.empty() || (vector.size() & (vector.size() - 1)) != 0) {
            return nullptr;
        }

        std::vector<stack_frame> s;
        size_t pos = 0;
        int const terminalLevel = m_nodes.get_var_count();

        while (pos < vector.size()) {
            node_t* u = m_nodes.make_terminal_node(vector[pos++]);
            s.push_back({u, terminalLevel});
            shrink(s);
        }

        return s.back().node;
    }

    /**
    * @brief Returns subsets of the family where variable `var` is present.
    *
    * Removes `var` from all resulting subsets (i.e., projects on var = 1).
    */
    auto subset1(node_t* node, int var) -> node_t* {
        return subset1(diagram_t(node), var);
    }
    
    /**
    * @brief Internal recursive implementation of subset1.
    */
    auto subset1(diagram_t const& diagram, int var) -> node_t* {
        node_t* root = diagram.unsafe_get_root();

        auto* cached = m_unary_cache.find(ops::ZDD_SUBSET1::get_id(), root, var);
        if (cached != nullptr) {
            return cached->result;
        }

        node_t* result = nullptr;
        int32 index = get_index_safe(root);

        if (index > var) {
            result = m_nodes.make_terminal_node(0);
        }
        else if (index == var) {
            result = root->get_son(1);
        }
        else {
            auto* low = subset1(diagram_t(root->get_son(0)), var);
            auto* high = subset1(diagram_t(root->get_son(1)), var);
            result = get_node(index, low, high);
        }

        m_unary_cache.put(ops::ZDD_SUBSET1::get_id(), root, var, result, 0);
        return result;
    }

    /**
    * @brief Returns subsets of the family where variable `var` is absent.
    *
    * Keeps only subsets that do not contain `var`.
    */
    auto subset0(node_t* node, int var) -> node_t* {
        return subset0(diagram_t(node), var);
    }

    /**
    * @brief Internal recursive implementation of subset0.
    */
    auto subset0(diagram_t const& diagram, int var) -> node_t* {
        node_t* root = diagram.unsafe_get_root();

        auto* cached = m_unary_cache.find(ops::ZDD_SUBSET0::get_id(), root, var);
        if (cached != nullptr) {
            return cached->result;
        }

        node_t* result = nullptr;
        int32 index = get_index_safe(root);

        if (index > var) {
            result = root;
        }
        else if (index == var) {
            result = root->get_son(0);
        }
        else {
            auto* low = subset0(diagram_t(root->get_son(0)), var);
            auto* high = subset0(diagram_t(root->get_son(1)), var);
            result = get_node(index, low, high);
        }

        m_unary_cache.put(ops::ZDD_SUBSET0::get_id(), root, var, result, 0);
        return result;
    }

    /**
    * @brief Toggles presence of variable `var` in all subsets.
    *
    * If `var` is present in a subset, it is removed; otherwise, it is added.
    */
    auto change(node_t* node, int var) -> node_t* {
        return change(diagram_t(node), var);
    }

    /**
    * @brief Internal recursive implementation of change.
    */
    auto change(diagram_t const& diagram, int var) -> node_t* {
        node_t* root = diagram.unsafe_get_root();

        auto* cached = m_unary_cache.find(ops::ZDD_CHANGE::get_id(), root, var);
        if (cached != nullptr) {
            return cached->result;
        }

        node_t* result = nullptr;
        int32 index = get_index_safe(root);

        if (index > var) {
            result = get_node(var, m_nodes.make_terminal_node(0), root);
        }
        else if (index == var) {
            result = get_node(var, root->get_son(1), root->get_son(0));
        }
        else {
            auto* low  = change(diagram_t(root->get_son(0)), var);
            auto* high = change(diagram_t(root->get_son(1)), var);
            result = get_node(index, low, high);
        }
        
        m_unary_cache.put(ops::ZDD_CHANGE::get_id(), root, var, result, 0);
        return result;
    }

    /**
    * @brief Computes union of two ZDDs (set of sets).
    *
    * Result represents all subsets that are in either P or Q.
    */
    auto unification(node_t* P, node_t* Q) -> node_t* {
        return unification(diagram_t(P), diagram_t(Q));
    }

    /**
    * @brief Internal recursive implementation of union (ZDD apply).
    */
    auto unification(diagram_t const& dP, diagram_t const& dQ) -> node_t* {
        node_t* P = dP.unsafe_get_root();
        node_t* Q = dQ.unsafe_get_root();

        auto* cached = m_nodes.cache_find<ops::ZDD_UNION>(P, Q);

        if (cached != nullptr) {
            return cached;
        }

        node_t* result = nullptr;

        // ∅ ∪ Q = Q
        if (P->is_terminal() && P->get_value() == 0) {
            result = Q;
        }
        else if ((Q->is_terminal() && Q->get_value() == 0) || P == Q) { // P ∪ ∅ = P, P ∪ P = P
            result = P;
        }

        if (result != nullptr) {
            m_nodes.cache_put<ops::ZDD_UNION>(result, P, Q);
            return result;
        }

        int pIndex = get_index_safe(P);
        int qIndex = get_index_safe(Q);

        if (pIndex > qIndex) {
            auto* low  = unification(diagram_t(P), diagram_t(get_low(Q)));
            auto* high = get_high(Q);
            result = get_node(qIndex, low, high);
        }
        else if (pIndex < qIndex) {
            auto* low  = unification(diagram_t(get_low(P)), diagram_t(Q));
            auto* high = get_high(P);
            result = get_node(pIndex, low, high);
        }
        else {
            auto* low  = unification(diagram_t(get_low(P)), diagram_t(get_low(Q)));
            auto* high = unification(diagram_t(get_high(P)), diagram_t(get_high(Q)));
            result = get_node(pIndex, low, high);
        }
        
        m_nodes.cache_put<ops::ZDD_UNION>(result, P, Q);
        return result;
    }

    /**
    * @brief Computes intersection of two ZDDs.
    *
    * Result contains only subsets present in both P and Q.
    */
    auto intersect(node_t* P, node_t* Q) -> node_t* {
        return intersect(diagram_t(P), diagram_t(Q));
    }

    /**
    * @brief Internal recursive implementation of intersection.
    */
    auto intersect(diagram_t const& dP, diagram_t const& dQ) -> node_t* {
        node_t* P = dP.unsafe_get_root();
        node_t* Q = dQ.unsafe_get_root();

        auto* cached = m_nodes.cache_find<ops::ZDD_INTERSECT>(P, Q);
        if (cached != nullptr) {
            return cached;
        }

        node_t* result = nullptr;

        if ((P->is_terminal() && P->get_value() == 0) ||
            (Q->is_terminal() && Q->get_value() == 0)) {
            result = m_nodes.make_terminal_node(0);
        }
        else if (P == Q) {
            result = P;
        }

        if (result != nullptr) {
            m_nodes.cache_put<ops::ZDD_INTERSECT>(result, P, Q);
            return result;
        }

        int pIndex = get_index_safe(P);
        int qIndex = get_index_safe(Q);

        if (pIndex > qIndex) {
            result = intersect(dP, diagram_t(get_low(Q)));
        }
        else if (pIndex < qIndex) {
            result = intersect(diagram_t(get_low(P)), dQ);
        }
        else {
            auto* low  = intersect(diagram_t(get_low(P)), diagram_t(get_low(Q)));
            auto* high = intersect(diagram_t(get_high(P)), diagram_t(get_high(Q)));
            result = get_node(pIndex, low, high);
        }

        m_nodes.cache_put<ops::ZDD_INTERSECT>(result, P, Q);
        return result;
    }

    /**
    * @brief Computes set difference P \ Q.
    *
    * Removes all subsets from P that are also present in Q.
    */
    auto difference(node_t* P, node_t* Q) -> node_t* {
        return difference(diagram_t(P), diagram_t(Q));
    }

    /**
    * @brief Internal recursive implementation of difference.
    */
    auto difference(diagram_t const& dP, diagram_t const& dQ) -> node_t* {
        node_t* P = dP.unsafe_get_root();
        node_t* Q = dQ.unsafe_get_root();

        auto* cached = m_nodes.cache_find<ops::ZDD_DIFFERENCE>(P, Q);
        if (cached != nullptr) {
            return cached;
        }

        node_t* result = nullptr;

        if ((P->is_terminal() && P->get_value() == 0) || (Q->is_terminal() && Q->get_value() == 0)) {
            result = P;
        }
        else if (P == Q) {
            result = m_nodes.make_terminal_node(0);
        }

        if (result != nullptr) {
            m_nodes.cache_put<ops::ZDD_DIFFERENCE>(result, P, Q);
            return result;
        }

        int pIndex = get_index_safe(P);
        int qIndex = get_index_safe(Q);

        if (pIndex > qIndex) {
            result = difference(dP, diagram_t(get_low(Q)));
        }
        else if (pIndex < qIndex) {
            auto* low  = difference(diagram_t(get_low(P)), dQ);
            auto* high = get_high(P);
            result = get_node(pIndex, low, high);
        }
        else {
            auto* low  = difference(diagram_t(get_low(P)), diagram_t(get_low(Q)));
            auto* high = difference(diagram_t(get_high(P)), diagram_t(get_high(Q)));
            result = get_node(pIndex, low, high);
        }
        
        m_nodes.cache_put<ops::ZDD_DIFFERENCE>(result, P, Q);
        return result;
    }

    /**
    * @brief Counts number of subsets represented by the ZDD.
    *
    * Each path to terminal 1 corresponds to one subset.
    */
    auto count(node_t* node) -> int64 {
        return count(diagram_t(node));
    }

    /**
    * @brief Internal recursive implementation of count.
    */
    auto count(diagram_t const& diagram) -> int64 {
        node_t* node = diagram.unsafe_get_root();

        auto* cached = m_unary_cache.find(ops::ZDD_COUNT::get_id(), node, -1);
        if (cached != nullptr) {
            return cached->number;
        }

        int64 result = 0;

        if (node->is_terminal() && node->get_value() == 0) {
            result = 0;
        }
        else if (node->is_terminal() && node->get_value() == 1) {
            result = 1;
        }
        else {
            result = count(diagram_t(node->get_son(0))) + count(diagram_t(node->get_son(1)));
        }

        m_unary_cache.put(ops::ZDD_COUNT::get_id(), node, -1, nullptr, result);
        return result;
    }

    
    auto evaluate(node_t* const node, const std::vector<int>& values) -> int32 {
        return evaluate(diagram_t(node), values);
    }

    auto evaluate(diagram_t const& diagram, const std::vector<int>& values) -> int32 {
        node_t* node = diagram.unsafe_get_root();

        while (not node->is_terminal()) {
            int32 const index = node->get_index();
            assert(m_nodes.is_valid_var_value(index, values[static_cast<uint32>(index)]));
            node = node->get_son(values[static_cast<uint32>(index)]);
         }

        return node->get_value();
    }

    auto to_dot(diagram_t const& diagram) -> void {
        io::to_dot(m_nodes, std::cout, diagram);
    }

    auto to_dot(node_t* node) -> void {
        io::to_dot(m_nodes, std::cout, diagram_t(node));
    }


private:
    node_manager<degrees::fixed<DOMAIN_SIZE>, domains::fixed<DOMAIN_SIZE>> m_nodes;
    unary_cache m_unary_cache;
    using stack_frame   = struct {
        node_t* node;
        int32 level;
    };

    auto shrink(std::vector<stack_frame>& s) -> void {
        while(true) {
            if (s.size() < DOMAIN_SIZE) {
                return;
            }

            int32 const currentLevel = s.back().level;
            auto sons = node_t::make_son_container(DOMAIN_SIZE);

            for (int i = 0; i < DOMAIN_SIZE; ++i) {
                auto const& frame = s[s.size() - DOMAIN_SIZE + static_cast<size_t>(i)];

                if (frame.level != currentLevel) {
                    return;
                }

                sons[i] = frame.node;
            }

            s.resize(s.size() - DOMAIN_SIZE);

            int32 newIndex = m_nodes.get_index(currentLevel - 1);
            node_t* u = m_nodes.make_internal_node_zdd(newIndex, sons);
            s.push_back({u, currentLevel - 1});
        }
    }

    auto get_node(int index, node_t* low, node_t* high) -> node_t*
    {
        auto sons = node_t::make_son_container(DOMAIN_SIZE);
        sons[0] = low;
        sons[1] = high;

        return m_nodes.make_internal_node_zdd(index, sons);
    }

    auto static get_low(node_t* n) -> node_t* {
        if (n->is_terminal()) {
             return n;
        }
        return n->get_son(0);
    }

    auto static get_high(node_t* n) -> node_t* {
        if (n->is_terminal()) {
            return n;
        }
        return n->get_son(1);
    }

    auto static get_index_safe(node_t* n) -> int {
        return n->is_terminal() ? INT32_MAX : n->get_index();
    }
};

} // namespace teddy

#endif