#ifndef MIX_DD_MDD_MANAGER_HPP
#define MIX_DD_MDD_MANAGER_HPP

#include "diagrams/mdd.hpp"
#include "diagrams/vertex_memo.hpp"
#include "diagrams/vertex_manager.hpp"
#include "diagrams/var_vals.hpp"
#include <array>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    class mdd_manager
    {
    /* Public aliases */
    public:
        using mdd_t      = mdd<VertexData, ArcData, P>;
        using mdd_v      = std::vector<mdd_t>;
        using vertex_t   = vertex<VertexData, ArcData, P>;
        using log_t      = typename log_val_traits<P>::type;
        using log_v      = std::vector<log_t>;
        using prob_table = std::vector<std::array<double, P>>;
        using double_v   = std::vector<double>;

    /* Constructors*/
    public:
        mdd_manager (std::size_t const varCount);

    /* Tools */
    public:
        auto vertex_count  (mdd_t const& d) const -> std::size_t;
        auto to_dot_graph  (std::ostream& ost) const -> void;
        auto to_dot_graph  (std::ostream& ost, mdd_t const& d) const -> void;
        auto satisfy_count (log_t const val, mdd_t& d) -> std::size_t;

        template<class VertexOp>
        auto traverse_pre (mdd_t const& d, VertexOp&& op) const -> void;

        template<class VertexOp>
        auto traverse_post (mdd_t const& d, VertexOp&& op) const -> void;

        template<class VertexOp>
        auto traverse_level (mdd_t const& d, VertexOp&& op) const -> void;

    /* Manipulation */
    public:
        template<class Op>
        auto apply (mdd_t const& lhs, Op op, mdd_t const& rhs) -> mdd_t;

        auto restrict_var (mdd_t const& d, index_t const i, log_t const val) -> mdd_t;

        template<class Op>
        auto left_fold (mdd_v mdds, Op op) -> mdd_t;

        template<class Op>
        auto tree_fold (mdd_v mdds, Op op) -> mdd_t;

        template<class InputIt, class Op>
        auto left_fold (InputIt first, InputIt last, Op op) -> mdd_t;

        template<class RandomIt, class Op>
        auto tree_fold (RandomIt first, RandomIt last, Op op) -> mdd_t;

    /* Creation */
    public:
        auto just_val   (log_t const val) -> mdd_t;
        auto just_var   (index_t const i) -> mdd_t;
        auto operator() (index_t const i) -> mdd_t;
        auto just_var   (index_t const i, log_t const domain) -> mdd_t;
        auto operator() (index_t const i, log_t const domain) -> mdd_t;

    /* Reliability */
    public:
        auto calculate_probabilities (prob_table const& ps, mdd_t& f)                    -> void;
        auto get_probability         (log_t const level) const                           -> double;
        auto get_availability        (log_t const level) const                           -> double;
        auto get_unavailability      (log_t const level) const                           -> double;
        auto availability            (log_t const level, prob_table const& ps, mdd_t& f) -> double;
        auto unavailability          (log_t const level, prob_table const& ps, mdd_t& f) -> double;

        auto dpbd              (val_change<P> const var, val_change<P> const f, mdd_t const& sf, index_t const i) -> mdd_t;
        auto dpbd_integrated_1 (val_change<P> const var, log_t const fVal, mdd_t const& sf, index_t const i)      -> mdd_t;
        auto dpbd_integrated_2 (val_change<P> const var, mdd_t const& sf, index_t const i)                        -> mdd_t;
        auto dpbd_integrated_3 (val_change<P> const var, log_t const fVal, mdd_t const& sf, index_t const i)      -> mdd_t;

        auto dpbds              (val_change<P> const var, val_change<P> const f, mdd_t const& sf) -> mdd_v;
        auto dpbds_integrated_1 (val_change<P> const var, log_t const fVal, mdd_t const& sf)      -> mdd_v;
        auto dpbds_integrated_2 (val_change<P> const var, mdd_t const& sf)                        -> mdd_v;
        auto dpbds_integrated_3 (val_change<P> const var, log_t const fVal, mdd_t const& sf)      -> mdd_v;

        auto structural_importance  (std::size_t const domainSize, mdd_t& dpbd)          -> double;
        auto structural_importance  (log_v const& domains, mdd_t& dpbd, index_t const i) -> double;
        auto structural_importances (log_v const& domains, mdd_v& dpbds)                 -> double_v;

        auto birnbaum_importance  (prob_table const& ps, mdd_t& dpbd)  -> double;
        auto birnbaum_importances (prob_table const& ps, mdd_v& dpbds) -> double_v;

    /* Internal aliases */
    protected:
        using manager_t        = vertex_manager<VertexData, ArcData, P>;
        using son_a            = std::array<vertex_t*, P>;
        using vertex_v         = std::vector<vertex_t*>;
        using vertex_vv        = std::vector<vertex_v>;
        using trans_id_t       = std::int16_t;
        using apply_key_t      = std::tuple<vertex_t*, op_id_t, vertex_t*>;
        using apply_memo_t     = vertex_memo<VertexData, ArcData, P, apply_key_t>;
        using transform_key_t  = std::tuple<vertex_t*>; // TODO tmp just pointer
        using transform_memo_t = vertex_memo<VertexData, ArcData, P, transform_key_t>;

    /* Tools internals */
    private:
        template<class LevelItPair>
        auto to_dot_graph_impl (std::ostream& ost, std::vector<LevelItPair> levels) const -> void;

        auto fill_levels (mdd_t const& d) const -> vertex_vv;

        template<class VertexOp>
        auto traverse_pre (vertex_t* const v, VertexOp&& op) const -> void;

        template<class VertexOp>
        auto traverse_post (vertex_t* const v, VertexOp&& op) const -> void;

    /* Manipulation internals */
    protected:
        template<class Op>
        auto apply_step (vertex_t* const lhs, Op op, vertex_t* const rhs) -> vertex_t*;

        template<class Op>
        static auto make_apply_key (vertex_t* const lhs, Op op, vertex_t* const rhs) -> apply_key_t;

        template<class Transformator>
        auto transform (mdd_t const& d, Transformator&& transform_sons) -> mdd_t;

        template<class Transformator>
        auto transform_step (vertex_t* const v, Transformator&& transform_sons) -> vertex_t*;

    /* Creator internals */
    protected:
        template<class LeafVals>
        auto just_var_impl (index_t const i, LeafVals&& vals) -> mdd_t;

    /* Reliability internals */
    private:
        auto sum_terminals (log_t const from, log_t const to) const -> double;

    /* Other internals */
    private:
        auto get_var_count() const -> std::size_t;

    /* Member variables */
    protected:
        manager_t        vertexManager_;
        apply_memo_t     applyMemo_;
        transform_memo_t transformMemo_;
    };
}

#include "diagrams/mdd_manager.ipp"
#include "diagrams/mdd_manager_tools.ipp"
#include "diagrams/mdd_manager_manipulator.ipp"
#include "diagrams/mdd_manager_creator.ipp"
#include "diagrams/mdd_manager_reliability.ipp"

#endif