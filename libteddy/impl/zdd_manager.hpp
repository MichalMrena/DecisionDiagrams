#ifndef LIBTEDDY_DETAILS_ZDD_MANAGER_HPP
#define LIBTEDDY_DETAILS_ZDD_MANAGER_HPP

#include <libteddy/impl/diagram_manager.hpp>
#include <libteddy/impl/node_manager.hpp>
#include <libteddy/impl/node.hpp>
#include <libteddy/inc/io.hpp>
#include <iostream>
#include <libteddy/impl/operators.hpp>

//TOOD
// add caching for subset1, subset0, change

namespace teddy
{

#define DOMAIN_SIZE 2

namespace ops {
    struct ZDD_UNION : details::operation_info<101, true> {};
    struct ZDD_INTERSECT : details::operation_info<102, true> {};
    struct ZDD_DIFFERENCE : details::operation_info<103, false> {};
    struct ZDD_CHANGE : details::operation_info<104, false> {};
    struct ZDD_SUBSET1 : details::operation_info<105, false> {};
    struct ZDD_SUBSET0 : details::operation_info<106, false> {};
    struct ZDD_COUNT : details::operation_info<107, false> {};
} //namespace ops

class zdd_manager
{
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
    ){
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
        if (root->is_terminal()) {
            return m_nodes.make_terminal_node(0);
        }

        if (root->get_index() > var) {
            return m_nodes.make_terminal_node(0);
        }

        if (root->get_index() == var) {
            return root->get_son(1);
        }

        auto* low = subset1(diagram_t(root->get_son(0)), var);
        auto* high = subset1(diagram_t(root->get_son(1)), var);

        return get_node(root->get_index(), low, high);
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
        if (root->is_terminal()) {
            return root;
        }

        if (root->get_index() > var) {
            return root;
        }

        if (root->get_index() == var) {
            return root->get_son(0);
        }

        auto* low = subset0(diagram_t(root->get_son(0)), var);
        auto* high = subset0(diagram_t(root->get_son(1)), var);

        return get_node(root->get_index(), low, high);
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

        if (root->is_terminal()) {
            if (root->get_value() == 0) {
                return root;
            }
            
            return get_node(var, m_nodes.make_terminal_node(0), m_nodes.make_terminal_node(1));
        }

        if (root->get_index() > var) {
            return get_node(var, m_nodes.make_terminal_node(0), root);
        }

        if (root->get_index() == var) {
            return get_node(var, root->get_son(1), root->get_son(0));
        }

        auto* low  = change(diagram_t(root->get_son(0)), var);
        auto* high = change(diagram_t(root->get_son(1)), var);

        return get_node(root->get_index(), low, high);
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

        if (cached) { //NOLINT
            return cached;
        }

        // ∅ ∪ Q = Q
        if (P->is_terminal() && P->get_value() == 0) {
            m_nodes.cache_put<ops::ZDD_UNION>(Q, P, Q);
            return Q;
        }
        
        // P ∪ ∅ = P
        if (Q->is_terminal() && Q->get_value() == 0) {
            m_nodes.cache_put<ops::ZDD_UNION>(P, P, Q);
            return P;
        }

        // {∅} ∪ Q
        //optimizaion for the common case of union with empty set - if P is {∅}, we can just add empty set to Q without recursion
        if (P->is_terminal() && P->get_value() == 1) {
            // {∅} ∪ {∅} = {∅}
            if (Q->is_terminal()) {
                m_nodes.cache_put<ops::ZDD_UNION>(P, P, Q);
                return P;
            }
            
            //add empty set to Q
            //low = 1 (∅)
            auto* result = get_node(Q->get_index(), m_nodes.make_terminal_node(1), Q->get_son(1));
            m_nodes.cache_put<ops::ZDD_UNION>(result, P, Q);
            return result;
        }

        // P ∪ {∅}  
        if (Q->is_terminal() && Q->get_value() == 1) {
            // {∅} ∪ {∅} = {∅}
            if (P->is_terminal()) {
                m_nodes.cache_put<ops::ZDD_UNION>(Q, P, Q);
                return Q;
            }

            auto* result = get_node(P->get_index(), m_nodes.make_terminal_node(1), P->get_son(1));
            m_nodes.cache_put<ops::ZDD_UNION>(result, P, Q);
            return result;
        }

        if (P == Q) {
            m_nodes.cache_put<ops::ZDD_UNION>(P, P, Q);
            return P;
        }

        int pIndex = get_index_safe(P);
        int qIndex = get_index_safe(Q);

        if (pIndex > qIndex) {
            auto* low  = unification(diagram_t(P), diagram_t(get_low(Q)));
            auto* high = get_high(Q);
            auto* result = get_node(qIndex, low, high);
            m_nodes.cache_put<ops::ZDD_UNION>(result, P, Q);

            return result;
        }

        if (pIndex < qIndex) {
            auto* low  = unification(diagram_t(get_low(P)), diagram_t(Q));
            auto* high = get_high(P);
            auto* result = get_node(pIndex, low, high);
            m_nodes.cache_put<ops::ZDD_UNION>(result, P, Q);

            return result;
        }

        auto* low  = unification(diagram_t(get_low(P)), diagram_t(get_low(Q)));
        auto* high = unification(diagram_t(get_high(P)), diagram_t(get_high(Q)));
        auto* result = get_node(pIndex, low, high);
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
        if (cached) { //NOLINT
            return cached;
        }

        if ((P->is_terminal() && P->get_value() == 0) ||
            (Q->is_terminal() && Q->get_value() == 0)) {
            auto* result = m_nodes.make_terminal_node(0);
            m_nodes.cache_put<ops::ZDD_INTERSECT>(result, P, Q);

            return result;
        }

        if (P->is_terminal() && P->get_value() == 1) {
            auto* result = contains_empty(Q) ? P : m_nodes.make_terminal_node(0);
            m_nodes.cache_put<ops::ZDD_INTERSECT>(result, P, Q);

            return result;
        }

        if (Q->is_terminal() && Q->get_value() == 1) {
            auto* result = contains_empty(P) ? Q : m_nodes.make_terminal_node(0);
            m_nodes.cache_put<ops::ZDD_INTERSECT>(result, P, Q);

            return result;
        }

        if (P == Q) {
            m_nodes.cache_put<ops::ZDD_INTERSECT>(P, P, Q);
            return P;
        }

        int pIndex = get_index_safe(P);
        int qIndex = get_index_safe(Q);

        if (pIndex > qIndex) {
            auto* result = intersect(diagram_t(get_low(P)), dQ);
            m_nodes.cache_put<ops::ZDD_INTERSECT>(result, P, Q);

            return result;
        }

        if (pIndex < qIndex) {
            auto* result = intersect(dP, diagram_t(get_low(Q)));
            m_nodes.cache_put<ops::ZDD_INTERSECT>(result, P, Q);

            return result;
        }

        auto* low  = intersect(diagram_t(get_low(P)), diagram_t(get_low(Q)));
        auto* high = intersect(diagram_t(get_high(P)), diagram_t(get_high(Q)));
        auto* result = get_node(pIndex, low, high);
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
        if (cached) { //NOLINT
            return cached;
        }

        if (P->is_terminal() && P->get_value() == 0) {
            m_nodes.cache_put<ops::ZDD_DIFFERENCE>(P, P, Q);
            return P;
        }

        if (Q->is_terminal() && Q->get_value() == 0) {
            m_nodes.cache_put<ops::ZDD_DIFFERENCE>(P, P, Q);
            return P;
        }

        if (P->is_terminal() && P->get_value() == 1) {
            auto* result = contains_empty(Q) ? m_nodes.make_terminal_node(0) : P;
            m_nodes.cache_put<ops::ZDD_DIFFERENCE>(result, P, Q);

            return result;
        }

        if (Q->is_terminal() && Q->get_value() == 1) {
            auto* result = remove_empty(P);
            m_nodes.cache_put<ops::ZDD_DIFFERENCE>(result, P, Q);

            return result;
        }

        if (P == Q) {
            auto* result = m_nodes.make_terminal_node(0);
            m_nodes.cache_put<ops::ZDD_DIFFERENCE>(result, P, Q);
            return result;
        }

        int pIndex = get_index_safe(P);
        int qIndex = get_index_safe(Q);

        if (pIndex > qIndex) {
            auto* low  = difference(diagram_t(get_low(P)), dQ);
            auto* high = get_high(P);
            auto* result = get_node(pIndex, low, high);
            m_nodes.cache_put<ops::ZDD_DIFFERENCE>(result, P, Q);

            return result;
        }

        if (pIndex < qIndex) {
            auto* result = difference(dP, diagram_t(get_low(Q)));
            m_nodes.cache_put<ops::ZDD_DIFFERENCE>(result, P, Q);
            
            return result;
        }

        auto* low  = difference(diagram_t(get_low(P)), diagram_t(get_low(Q)));
        auto* high = difference(diagram_t(get_high(P)), diagram_t(get_high(Q)));
        auto* result = get_node(pIndex, low, high);
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
        if (node->is_terminal() && node->get_value() == 0) {
            return 0;
        }

        if (node->is_terminal() && node->get_value() == 1) {
            return 1;
        }

        return count(node->get_son(0)) + count(node->get_son(1));
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

    auto get_high(node_t* n) -> node_t* {
        if (n->is_terminal()) {
            return m_nodes.make_terminal_node(0);
        }
        return n->get_son(1);
    }

    auto static get_index_safe(node_t* n) -> int {
        return n->is_terminal() ? INT32_MAX : n->get_index();
    }

    auto static contains_empty(node_t* n) -> bool {
        while (!n->is_terminal()) {
            n = n->get_son(0);
        }
        return n->get_value() == 1;
    }

    auto remove_empty(node_t* n) -> node_t* {
        if (n->is_terminal()) {
            return m_nodes.make_terminal_node(0);
        }

        auto* low  = remove_empty(n->get_son(0));
        auto* high = n->get_son(1);

        return get_node(n->get_index(), low, high);
    }
};

} // namespace teddy

#endif