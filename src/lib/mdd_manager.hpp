#ifndef MIX_DD_MDD_MANAGER_HPP
#define MIX_DD_MDD_MANAGER_HPP

#include "diagrams/mdd.hpp"
#include "diagrams/vertex_manager.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    class mdd_manager
    {
    /* Public aliases */
    public:
        using mdd_t    = mdd<VertexData, ArcData, P>;
        using mdd_v    = std::vector<mdd_t>;
        using vertex_t = vertex<VertexData, ArcData, P>;
        using log_t    = typename log_val_traits<P>::type;

    /* Constructors*/
    public:
        mdd_manager (std::size_t const varCount);

    /* Tools */
    public:
        auto vertex_count (mdd_t const& diagram) const -> std::size_t;
        auto to_dot_graph (std::ostream& ost) const -> void;
        auto to_dot_graph (std::ostream& ost, mdd_t const& diagram) const -> void;

        template<class VertexOp>
        auto traverse_pre (mdd_t const& diagram, VertexOp&& op) const -> void;

        template<class VertexOp>
        auto traverse_level (mdd_t const& diagram, VertexOp&& op) const -> void;

    /* Manipulation */
    public:
        template<class Op>
        auto apply (mdd_t const& lhs, Op op, mdd_t const& rhs) -> mdd_t;

        // default Proj -> identity (std::identity since C++20), veľmi výhodné pri posúvaní negovanej funckie
        template<class Op, class LhsProj, class RhsProj>
        auto apply (mdd_t const& lhs, Op op, mdd_t const& rhs) -> mdd_t;

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

    /* Reliability */
    public:

    /* Internal aliases */
    private:
        using manager_t = vertex_manager<VertexData, ArcData, P>;
        using vertex_v  = std::vector<vertex_t*>;
        using vertex_vv = std::vector<vertex_v>;
            using vertex_p = std::pair<vertex_t*, vertex_t*>;
            using vertex_m = std::unordered_map<vertex_p, vertex_t*, utils::tuple_hash_t<vertex_p>>;

    /* Tools internals */
    private:
        template<class LevelItPair>
        auto to_dot_graph_impl (std::ostream& ost, std::vector<LevelItPair> levels) const -> void;

        auto fill_levels (mdd_t const& diagram) const -> vertex_vv;

        template<class VertexOp>
        auto traverse_pre (vertex_t* const v, VertexOp&& op) const -> void;

    /* Manipulation internals */
    private:
        template<class Op>
        auto apply_step (vertex_t* const lhs, Op op, vertex_t* const rhs) -> vertex_t*;

        template<class ValOp>
        auto map (mdd_t const& d, ValOp op) -> mdd_t; // apply op on terminal vertices (not for bdd)

        template<class VertexOp>
        auto transform (mdd_t const& d, VertexOp op) -> mdd_t; // apply op on internal vertices (restrict, dpbde, mnf)

    /* Creator internals */
    private:
        template<class LeafVals>
        auto just_var_impl (index_t const i, LeafVals&& vals) -> mdd_t;

    /* Member variables */
    protected:
        manager_t manager_;
            vertex_m   memo_; // TODO apply memo
    };

    template<class VertexData, class ArcData, std::size_t P>
    mdd_manager<VertexData, ArcData, P>::mdd_manager
        (std::size_t const varCount) :
        manager_ {varCount}
    {
    }
}

#include "diagrams/mdd_manager_tools.ipp"
#include "diagrams/mdd_manager_manipulator.ipp"
#include "diagrams/mdd_manager_creator.ipp"

#endif