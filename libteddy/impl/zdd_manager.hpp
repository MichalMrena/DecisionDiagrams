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
    * @return Root node wrapped in a diagram_t.
    */
    auto from_vector(const std::vector<int>& vector) -> diagram_t {
        assert(!vector.empty() && (vector.size() & (vector.size() - 1)) == 0);

        std::vector<stack_frame> s;
        size_t pos = 0;
        int const terminalLevel = m_nodes.get_var_count();

        while (pos < vector.size()) {
            node_t* u = m_nodes.make_terminal_node(vector[pos++]);
            s.push_back({u, terminalLevel});
            shrink(s);
        }
        
        assert(s.size() == 1);
        return diagram_t(s.back().node);
    } 

    /**
    * @brief Returns subsets of the family where variable `var` is present.
    *
    * Removes `var` from all resulting subsets (i.e., projects on var = 1).
    */
    auto subset1(const diagram_t& node, int var) -> diagram_t {
        cleanup();
        return diagram_t(subset1(node.unsafe_get_root(), var));
    }

    /**
    * @brief Returns subsets of the family where variable `var` is absent.
    *
    * Keeps only subsets that do not contain `var`.
    */
    auto subset0(const diagram_t& node, int var) -> diagram_t {
        cleanup();
        return diagram_t(subset0(node.unsafe_get_root(), var));
    }

    /**
    * @brief Toggles presence of variable `var` in all subsets.
    *
    * If `var` is present in a subset, it is removed; otherwise, it is added.
    */
    auto change(const diagram_t& node, int var) -> diagram_t {
        cleanup();
        return diagram_t(change(node.unsafe_get_root(), var));
    }

    /**
    * @brief Computes union of two ZDDs (set of sets).
    *
    * Result represents all subsets that are in either P or Q.
    */
    auto unification(const diagram_t& P, const diagram_t& Q) -> diagram_t {
        cleanup();
        return diagram_t(unification(P.unsafe_get_root(), Q.unsafe_get_root()));
    }

    /**
    * @brief Computes intersection of two ZDDs.
    *
    * Result contains only subsets present in both P and Q.
    */
    auto intersect(const diagram_t& P, const diagram_t& Q) -> diagram_t {
        cleanup();
        return diagram_t(intersect(P.unsafe_get_root(), Q.unsafe_get_root()));
    }

    /**
    * @brief Computes set difference P \ Q.
    *
    * Removes all subsets from P that are also present in Q.
    */
    auto difference(const diagram_t& P, const diagram_t& Q) -> diagram_t {
        cleanup();
        return diagram_t(difference(P.unsafe_get_root(), Q.unsafe_get_root()));
    }

    /**
    * @brief Counts number of subsets represented by the ZDD.
    *
    * Each path to terminal 1 corresponds to one subset.
    */
    auto count(const diagram_t& node) -> int64 {
        cleanup();
        return count(node.unsafe_get_root());
    }

    /**
    * @brief Returns the base ZDD (containing only the empty set).
    */
    auto base() -> diagram_t {
        return diagram_t(m_nodes.make_terminal_node(1));
    }

    /**
    * @brief Returns the empty ZDD (containing no sets).
    */
    auto empty() -> diagram_t {
        return diagram_t(m_nodes.make_terminal_node(0));
    }

    /**
    * @brief Evaluates the ZDD at a given assignment of variable values.
    *
    * @param diagram The ZDD to evaluate.
    * @param values The assignment of variable values.
    * @return The result of the evaluation (0 or 1).
    */
    auto evaluate(diagram_t const& diagram, const std::vector<int>& values) -> int32 {
        node_t* node = diagram.unsafe_get_root();

        while (not node->is_terminal()) {
            int32 const index = node->get_index();
            assert(m_nodes.is_valid_var_value(index, values[static_cast<uint32>(index)]));
            node = node->get_son(values[static_cast<uint32>(index)]);
         }

        return node->get_value();
    }

    /**
    * @brief Writes the ZDD in DOT format to console.
    *
    * @param diagram The ZDD to write.
    */
    auto to_dot(diagram_t const& diagram) -> void {
        io::to_dot(m_nodes, std::cout, diagram);
    }

    /**
    * @brief Writes the ZDD in DOT format to console.
    *
    * @param node The root node of the ZDD to write.
    */
    auto to_dot(node_t* node) -> void {
        io::to_dot(m_nodes, std::cout, diagram_t(node));
    }

    /**
    * @brief Returns the number of nodes in node manager.
    *
    * @return The number of nodes.
    */
    auto get_node_count() const -> int64 {
        return m_nodes.get_node_count();
    };

    /**
    * @brief Returns the number of nodes in the ZDD.
    *
    * @param diagram The ZDD.
    * @return The number of nodes.
    */
    auto get_node_count(diagram_t const& diagram) const -> int64 {
        return m_nodes.get_node_count(diagram.unsafe_get_root());
    }

private:
    node_manager<degrees::fixed<DOMAIN_SIZE>, domains::fixed<DOMAIN_SIZE>> m_nodes;
    unary_cache m_unary_cache;
    using stack_frame   = struct {
        node_t* node;
        int32 level;
    };

    auto cleanup() -> void {
        m_nodes.run_deferred();
        m_unary_cache.clear();
    }

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

    auto subset1(node_t* root, int var) -> node_t* {
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
            auto* low = subset1(root->get_son(0), var);
            auto* high = subset1(root->get_son(1), var);
            result = get_node(index, low, high);
        }

        m_unary_cache.put(ops::ZDD_SUBSET1::get_id(), root, var, result, 0);
        return result;
    }

        auto subset0(node_t* root, int var) -> node_t* {
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
            auto* low = subset0(root->get_son(0), var);
            auto* high = subset0(root->get_son(1), var);
            result = get_node(index, low, high);
        }

        m_unary_cache.put(ops::ZDD_SUBSET0::get_id(), root, var, result, 0);
        return result;
    }

    auto change(node_t* root, int var) -> node_t* {
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
            auto* low  = change(root->get_son(0), var);
            auto* high = change(root->get_son(1), var);
            result = get_node(index, low, high);
        }
        
        m_unary_cache.put(ops::ZDD_CHANGE::get_id(), root, var, result, 0);
        return result;
    }

        auto unification(node_t* P, node_t* Q) -> node_t* {
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
            auto* low  = unification(P, get_low(Q));
            auto* high = get_high(Q);
            result = get_node(qIndex, low, high);
        }
        else if (pIndex < qIndex) {
            auto* low  = unification(get_low(P), Q);
            auto* high = get_high(P);
            result = get_node(pIndex, low, high);
        }
        else {
            auto* low  = unification(get_low(P), get_low(Q));
            auto* high = unification(get_high(P), get_high(Q));
            result = get_node(pIndex, low, high);
        }
        
        m_nodes.cache_put<ops::ZDD_UNION>(result, P, Q);
        return result;
    }

        auto intersect(node_t* P, node_t* Q) -> node_t* {
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
            result = intersect(P, get_low(Q));
        }
        else if (pIndex < qIndex) {
            result = intersect(get_low(P), Q);
        }
        else {
            auto* low  = intersect(get_low(P), get_low(Q));
            auto* high = intersect(get_high(P), get_high(Q));
            result = get_node(pIndex, low, high);
        }

        m_nodes.cache_put<ops::ZDD_INTERSECT>(result, P, Q);
        return result;
    }

        auto difference(node_t* P, node_t* Q) -> node_t* {
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
            result = difference(P, get_low(Q));
        }
        else if (pIndex < qIndex) {
            auto* low  = difference(get_low(P), Q);
            auto* high = get_high(P);
            result = get_node(pIndex, low, high);
        }
        else {
            auto* low  = difference(get_low(P), get_low(Q));
            auto* high = difference(get_high(P), get_high(Q));
            result = get_node(pIndex, low, high);
        }
        
        m_nodes.cache_put<ops::ZDD_DIFFERENCE>(result, P, Q);
        return result;
    }

        auto count(node_t* root) -> int64 {
        auto* cached = m_unary_cache.find(ops::ZDD_COUNT::get_id(), root, -1);
        if (cached != nullptr) {
            return cached->number;
        }

        int64 result = 0;

        if (root->is_terminal() && root->get_value() == 0) {
            result = 0;
        }
        else if (root->is_terminal() && root->get_value() == 1) {
            result = 1;
        }
        else {
            result = count(root->get_son(0)) + count(root->get_son(1));
        }

        m_unary_cache.put(ops::ZDD_COUNT::get_id(), root, -1, nullptr, result);
        return result;
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