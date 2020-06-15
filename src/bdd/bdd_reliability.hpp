#ifndef MIX_DD_BDD_RELIABILITY_
#define MIX_DD_BDD_RELIABILITY_

#include "bdd.hpp"
#include "bdd_manipulator.hpp"
#include "../utils/hash.hpp"

#include <limits>
#include <utility>

namespace mix::dd
{
    template<class VertexData, class ArcData>
    class bdd_reliability
    {
    public:
        using bdd_t    = bdd<VertexData, ArcData>;
        using vertex_t = typename bdd_t::vertex_t;
        using arc_t    = typename bdd_t::arc_t;

    public:
        auto dpbd   (bdd_t structureFunc, index_t const i)                    -> bdd_t;
        auto dpbd_e (bdd_t structureFunc, index_t const i, bool_t const from) -> bdd_t;

    private:
        using manipulator_t = bdd_manipulator<VertexData, ArcData>;
        using vertex_pair_t = std::pair<const vertex_t*, const vertex_t*>;
        using memo_map      = std::unordered_map<const vertex_pair_t, vertex_t*, utils::tuple_hash_t<const vertex_pair_t>>;

    private:
        auto to_dpbd_e      (bdd_t d, index_t const i, bool_t const from)                     -> bdd_t;
        auto find_positions (bdd_t const& d, index_t const i)                                 -> std::vector<std::pair<vertex_t*, bool_t>>;
        auto new_vertex     ( vertex_t* const low
                            , vertex_t* const high
                            , index_t const   i
                            , bool const      mark) -> vertex_t*;

    private:
        manipulator_t manipulator_;
        memo_map      memo_;
        id_t          nextId_;
    };

    template<class VertexData, class ArcData>
    auto bdd_reliability<VertexData, ArcData>::dpbd   
        (bdd_t structureFunc, index_t const i) -> bdd_t
    {
        auto sfCopy = structureFunc.clone();
        return manipulator_.apply( manipulator_.negate(manipulator_.restrict_var(std::move(structureFunc), i, 0))
                                 , AND {}
                                 , manipulator_.restrict_var(std::move(sfCopy), i, 1) );
    }

    template<class VertexData, class ArcData>
    auto bdd_reliability<VertexData, ArcData>::dpbd_e   
        (bdd_t structureFunc, index_t const i, bool_t const from) -> bdd_t
    {
        return to_dpbd_e(dpbd(structureFunc, i), i, from);
    }

    template<class VertexData, class ArcData>
    auto bdd_reliability<VertexData, ArcData>::to_dpbd_e
        (bdd_t d, index_t const i, bool_t const from) -> bdd_t
    {
        nextId_                    = std::numeric_limits<id_t>::max();
        auto const insertPositions = find_positions(d, i);
        auto const undefinedLeaf   = this->new_vertex(nullptr, nullptr, d.leaf_index(), d.root_->mark);
        d.leafToVal.emplace(undefinedLeaf, log_val_traits<2>::undefined);

        for (auto [vertex, sonIndex] : insertPositions)
        {
            auto const target     = vertex->son(sonIndex);
            auto const low        = 0 == from ? target : undefinedLeaf;
            auto const high       = 0 == from ? undefinedLeaf : target;
            vertex->son(sonIndex) = this->new_vertex(low, high, i, d.root_->mark);
        }        

        if (0 == i)
        {
            auto const low  = 0 == from ? d.root_ : undefinedLeaf;
            auto const high = 0 == from ? undefinedLeaf : d.root_;
            d.root_         = this->new_vertex(low, high, i, d.root_->mark);
        }

        memo_.clear();
        return d;
    }

    template<class VertexData, class ArcData>
    auto bdd_reliability<VertexData, ArcData>::find_positions
        (bdd_t const& d, index_t const i) -> std::vector<std::pair<vertex_t*, bool_t>>
    {
        auto positions = std::vector<std::pair<vertex_t*, log_val_traits<2>::value_t>> {};

        d.traverse(d.root_, [&d, &positions, i](auto const v)
        {
            if (d.is_leaf(v) || v->index > i)
            {
                return;
            }

            if (v->son(0)->index > i)
            {
                positions.emplace_back(v, 0);
            }

            if (v->son(1)->index > i)
            {
                positions.emplace_back(v, 1);
            }
        });

        return positions;
    }

    template<class VertexData, class ArcData>
    auto bdd_reliability<VertexData, ArcData>::new_vertex
        (vertex_t* const low, vertex_t* const high, index_t const i, bool const mark) -> vertex_t*
    {
        using arr_t = typename vertex_t::forward_star_arr;

        auto const key = vertex_pair_t {low, high};
        auto const it  = memo_.find(key);
        
        if (it != memo_.end())
        {
            it->second->mark = mark;
            return it->second;
        }

        auto const v = new vertex_t {nextId_--, i, arr_t {arc_t {low}, arc_t {high}}};
        v->mark = mark;
        memo_.emplace(key, v);

        return v;
    }
}

#endif