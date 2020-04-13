#ifndef MIX_DD_BDD_MANIPULATOR_
#define MIX_DD_BDD_MANIPULATOR_

#include <unordered_map>
#include <map>
#include <algorithm>
#include <tuple>
#include "bdd.hpp"
#include "operators.hpp"
#include "../dd/dd_manipulator_base.hpp"
#include "../utils/hash.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData>
    class bdd_manipulator : public dd_manipulator_base<VertexData, ArcData, 2>
    {
    public:
        using bdd_t    = bdd<VertexData, ArcData>;
        using vertex_t = typename bdd_t::vertex_t;
        using arc_t    = typename bdd_t::arc_t;

    public:
        template<class BinOp> auto apply (bdd_t&&      d1, BinOp op, bdd_t&&      d2) -> bdd_t;
        template<class BinOp> auto apply (const bdd_t& d1, BinOp op, bdd_t&&      d2) -> bdd_t;
        template<class BinOp> auto apply (bdd_t&&      d1, BinOp op, const bdd_t& d2) -> bdd_t;
        template<class BinOp> auto apply (const bdd_t& d1, BinOp op, const bdd_t& d2) -> bdd_t;

        auto restrict_var (bdd_t&  diagram, const index_t i, const bool_t val) -> bdd_t&;
        auto restrict_var (bdd_t&& diagram, const index_t i, const bool_t val) -> bdd_t;

        auto negate (bdd_t&  diagram) -> bdd_t&;
        auto negate (bdd_t&& diagram) -> bdd_t;

        auto reduce (bdd_t&  diagram) -> bdd_t&;
        auto reduce (bdd_t&& diagram) -> bdd_t;

        virtual ~bdd_manipulator() = default;

    private:
        using vertex_pair_t      = std::pair<const vertex_t*, const vertex_t*>;
        using yet_in_triplet_t   = std::tuple<index_t, id_t, id_t>;
        using recursion_memo_map = std::unordered_map<const vertex_pair_t, vertex_t*, utils::tuple_hash_t<const vertex_pair_t>>;
        using in_graph_memo_map  = std::unordered_map<const yet_in_triplet_t, vertex_t*, utils::tuple_hash_t<const yet_in_triplet_t>>;
        using leaf_val_map       = typename bdd_t::leaf_val_map;
        using val_leaf_arr       = std::array<vertex_t*, 2>;

        template<class BinOp>
        auto apply_step ( const vertex_t* const v1
                        , BinOp op
                        , const vertex_t* const v2 ) -> vertex_t*;

        auto leaf_index () const -> index_t;

        auto index1 (const vertex_t* const v1) const -> index_t;
        auto index2 (const vertex_t* const v2) const -> index_t;
        auto value1 (const vertex_t* const v1) const -> bool_t;
        auto value2 (const vertex_t* const v2) const -> bool_t;

        auto terminal_vertex ( const bool_t val ) -> vertex_t*;
        auto internal_vertex ( const index_t index
                             , vertex_t* const low
                             , vertex_t* const high ) -> vertex_t*;

        auto recycle (bdd_t& d) -> void;
        auto reset   ()         -> void;

        recursion_memo_map recursionMemo_;
        in_graph_memo_map  inGraphMemo_;
        leaf_val_map       leafToVal_;
        val_leaf_arr       valToLeaf_ {nullptr, nullptr};
        const bdd_t*       diagram1_  {nullptr};
        const bdd_t*       diagram2_  {nullptr};
        id_t               nextId_    {0};
    };

    template<class VertexData, class ArcData>
    auto operator&& ( bdd<VertexData, ArcData> lhs
                    , bdd<VertexData, ArcData> rhs ) -> bdd<VertexData, ArcData>;
    
    template<class VertexData, class ArcData>
    auto operator* ( bdd<VertexData, ArcData> lhs
                   , bdd<VertexData, ArcData> rhs ) -> bdd<VertexData, ArcData>;

    template<class VertexData, class ArcData>
    auto operator|| ( bdd<VertexData, ArcData> lhs
                    , bdd<VertexData, ArcData> rhs ) -> bdd<VertexData, ArcData>;

    template<class VertexData, class ArcData>
    auto operator+ ( bdd<VertexData, ArcData> lhs
                   , bdd<VertexData, ArcData> rhs ) -> bdd<VertexData, ArcData>;
    
    template<class VertexData, class ArcData>
    auto operator^ ( bdd<VertexData, ArcData> lhs
                   , bdd<VertexData, ArcData> rhs ) -> bdd<VertexData, ArcData>;
    
    template<class VertexData, class ArcData>
    auto operator! ( bdd<VertexData, ArcData> lhs ) -> bdd<VertexData, ArcData>;
    
    template<class VertexData, class ArcData>
    template<class BinOp>
    auto bdd_manipulator<VertexData, ArcData>::apply
        (bdd_t&& d1, BinOp op, bdd_t&& d2) -> bdd_t
    {
        auto newDiagram {this->apply(d1, op, d2)};
        this->recycle(d1);
        this->recycle(d2);
        return newDiagram;
    }
    
    template<class VertexData, class ArcData>
    template<class BinOp>
    auto bdd_manipulator<VertexData, ArcData>::apply
        (bdd_t&& d1, BinOp op, const bdd_t& d2) -> bdd_t
    {
        auto newDiagram {this->apply(d1, op, d2)};
        this->recycle(d1);
        return newDiagram;
    }
    
    template<class VertexData, class ArcData>
    template<class BinOp>
    auto bdd_manipulator<VertexData, ArcData>::apply
        (const bdd_t& d1, BinOp op, bdd_t&& d2) -> bdd_t
    {
        auto newDiagram {this->apply(d1, op, d2)};
        this->recycle(d2);
        return newDiagram;
    }
    
    template<class VertexData, class ArcData>
    template<class BinOp>
    auto bdd_manipulator<VertexData, ArcData>::apply
        (const bdd_t& d1, BinOp op, const bdd_t& d2) -> bdd_t
    {
        diagram1_ = &d1;
        diagram2_ = &d2;

        bdd_t newDiagram 
        {
            this->apply_step(d1.root_, op, d2.root_)
          , std::max(d1.variableCount, d2.variableCount)
          , std::move(leafToVal_)
        };

        this->reset();

        return newDiagram;
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::restrict_var
        (bdd_t& diagram, const index_t i, const bool_t val) -> bdd_t&
    {
        if (i >= diagram.variableCount)
        {
            return diagram;
        }

        const auto oldVertices 
        {
            diagram.template fill_container< std::set<vertex_t*> >()
        };

        // "skip" all vertices with given index
        diagram.traverse(diagram.root_, [i, val](vertex_t* const v)
        {
            if (v->is_leaf())
            {
                return;
            }

            if (! v->son(0)->is_leaf() && i == v->son(0)->index)
            {
                v->son(0) = v->son(0)->son(val);
            }

            if (! v->son(1)->is_leaf() && i == v->son(1)->index)
            {
                v->son(1) = v->son(1)->son(val);
            }
        });   

        // possibly change the root
        if (i == diagram.root_->index)
        {
            diagram.root_ = diagram.root_->son(val);
        }

        // identify now unreachable vertices
        const auto newVertices 
        {
            diagram.template fill_container< std::set<vertex_t*> >()
        };
        std::vector<vertex_t*> unreachableVertices;
        std::set_difference( oldVertices.begin(), oldVertices.end()
                           , newVertices.begin(), newVertices.end()
                           , std::inserter(unreachableVertices, unreachableVertices.begin()) );

        // and release them
        for (auto v : unreachableVertices)
        {
            this->release_vertex(v);
        }     

        return this->reduce(diagram);
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::restrict_var
        (bdd_t&& diagram, const index_t i, const bool_t val) -> bdd_t
    {
        return bdd_t {std::move(this->restrict_var(diagram, i, val))};
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::negate
        (bdd_t& diagram) -> bdd_t&
    {
        auto const trueLeaf  = diagram.true_leaf();
        auto const falseLeaf = diagram.false_leaf();

        if (trueLeaf)
        {
            auto& val = diagram.leafToVal.at(trueLeaf);
            val = ! val;
        }

        if (falseLeaf)
        {
            auto& val = diagram.leafToVal.at(falseLeaf);
            val = ! val;
        }

        return diagram;
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::negate
        (bdd_t&& diagram) -> bdd_t
    {
        return bdd_t {std::move(this->negate(diagram))};
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::reduce
        (bdd_t& diagram) -> bdd_t&
    {
        using key_vertex_pair = std::pair< std::pair<id_t, id_t>, vertex_t*>;
        using not_used_v      = std::vector<vertex_t*>;
        using vertex_map      = std::unordered_map<id_t, vertex_t*>; 

        const auto levels {diagram.fill_levels()};
        not_used_v redundantVertices;
        vertex_map subgraph;
        nextId_ = 0;

        for (size_t i {levels.size()}; i > 0;)
        {
            --i;
            std::vector<key_vertex_pair> keyedVertices;
            
            for (vertex_t* u : levels[i])
            {
                if (diagram.is_leaf(u))
                {
                    keyedVertices.emplace_back( std::make_pair(diagram.value(u), -1), u );
                }
                else if (u->son(0)->id == u->son(1)->id)
                {
                    u->id = u->son(0)->id;
                    redundantVertices.push_back(u);
                }
                else
                {
                    keyedVertices.emplace_back( std::make_pair( u->son(0)->id, u->son(1)->id), u);
                }
            }

            std::sort(keyedVertices.begin(), keyedVertices.end());

            auto oldKey {std::make_pair(-1, -1)};

            for (auto& [key, u] : keyedVertices)
            {
                if (key == oldKey)
                {
                    u->id = nextId_;
                    redundantVertices.push_back(u);
                    if (diagram.is_leaf(u))
                    {
                        diagram.leafToVal.erase(u);
                    }
                }
                else
                {
                    ++nextId_;
                    u->id = nextId_;

                    subgraph.emplace(nextId_, u);
                    
                    if (! diagram.is_leaf(u))
                    {
                        u->son(0) = subgraph.at(u->son(0)->id);
                        u->son(1) = subgraph.at(u->son(1)->id);
                    }

                    oldKey = key;
                }
            }
        }
        
        diagram.root_ = subgraph.at(diagram.root_->id);

        for (auto v : redundantVertices)
        {
            this->release_vertex(v);
        }

        nextId_ = 0;

        return diagram;
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::reduce
        (bdd_t&& diagram) -> bdd_t
    {
        return bdd_t {std::move(this->reduce(diagram))};
    }

    template<class VertexData, class ArcData>
    template<class BinOp>
    auto bdd_manipulator<VertexData, ArcData>::apply_step 
        (const vertex_t* const v1, BinOp op, const vertex_t* const v2) -> vertex_t*
    {
        const auto uit {recursionMemo_.find(vertex_pair_t {v1, v2})};
        if (uit != recursionMemo_.end())
        {
            return (*uit).second;
        }

        const auto val {op(this->value1(v1), this->value2(v2))};
        vertex_t* u    {nullptr};

        if (val != X)
        {
            u = this->terminal_vertex(val);
        }
        else
        {
            const vertex_t* vlow1  {nullptr};
            const vertex_t* vlow2  {nullptr};
            const vertex_t* vhigh1 {nullptr};
            const vertex_t* vhigh2 {nullptr};

            const auto index {std::min(this->index1(v1), this->index2(v2))};
            if (this->index1(v1) == index)
            {
                vlow1  = v1->son(0);
                vhigh1 = v1->son(1);
            }
            else
            {
                vlow1  = v1;
                vhigh1 = v1;
            }

            if (this->index2(v2) == index)
            {
                vlow2  = v2->son(0);
                vhigh2 = v2->son(1);
            }
            else
            {
                vlow2  = v2;
                vhigh2 = v2;
            }

            u = this->internal_vertex( index
                                     , this->apply_step(vlow1, op, vlow2)
                                     , this->apply_step(vhigh1, op, vhigh2) );
        }

        recursionMemo_.emplace(vertex_pair_t {v1, v2}, u);

        return u;
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::leaf_index
        () const -> index_t
    {
        return std::max( diagram1_->leaf_index()
                       , diagram2_->leaf_index() );
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::index1
        (const vertex_t* const v1) const -> index_t
    {
        return diagram1_->is_leaf(v1) ? this->leaf_index()
                                      : v1->index;
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::index2
        (const vertex_t* const v2) const -> index_t
    {
        return diagram2_->is_leaf(v2) ? this->leaf_index()
                                      : v2->index;
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::value1
        (const vertex_t* const v1)  const -> bool_t
    {
        return diagram1_->value(v1);
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::value2
        (const vertex_t* const v2)  const -> bool_t
    {
        return diagram2_->value(v2);
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::terminal_vertex
        (const bool_t val) -> vertex_t*
    {
        if (! valToLeaf_[val])
        {
            valToLeaf_[val] = this->create_vertex(nextId_++, this->leaf_index());
            leafToVal_.emplace(valToLeaf_[val], val);
        }

        return valToLeaf_[val];
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::internal_vertex
        (const index_t index, vertex_t* const low, vertex_t* const high ) -> vertex_t*
    {
        using arr_t = typename vertex_t::forward_star_arr;

        if (low->id == high->id)
        {
            return low;
        }

        const auto key       {std::make_tuple(index, low->id, high->id)};
        const auto inGraphIt {inGraphMemo_.find(key)};
        
        if (inGraphIt != inGraphMemo_.end())
        {
            return (*inGraphIt).second;
        }

        const auto newVertex
        {
            this->create_vertex(nextId_++, index, arr_t{arc_t {low}, arc_t {high}})
        };

        inGraphMemo_.emplace(key, newVertex);

        return newVertex;
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::recycle
        (bdd_t& d) -> void
    {
        if (! d.root_)
        {
            return;
        }

        d.traverse(d.root_, [this](vertex_t* const v) 
        {
            this->release_vertex(v);
        });

        d.root_ = nullptr;
    }

    template<class VertexData, class ArcData>
    auto bdd_manipulator<VertexData, ArcData>::reset
        () -> void
    {
        recursionMemo_.clear();
        inGraphMemo_.clear();
        leafToVal_.clear();
        valToLeaf_ = {nullptr, nullptr};
        diagram1_  = nullptr;
        diagram2_  = nullptr;
        nextId_    = 0;
    }

    template<class VertexData, class ArcData>
    auto operator&& ( bdd<VertexData, ArcData> lhs
                    , bdd<VertexData, ArcData> rhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> manipulator;
        return manipulator.apply(std::move(lhs), AND {}, std::move(rhs));
    }

    template<class VertexData, class ArcData>
    auto operator* ( bdd<VertexData, ArcData> lhs
                   , bdd<VertexData, ArcData> rhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> manipulator;
        return manipulator.apply(std::move(lhs), AND {}, std::move(rhs));
    }
    
    template<class VertexData, class ArcData>
    auto operator|| ( bdd<VertexData, ArcData> lhs
                    , bdd<VertexData, ArcData> rhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> manipulator;
        return manipulator.apply(std::move(lhs), OR {}, std::move(rhs));
    }
    
    template<class VertexData, class ArcData>
    auto operator+ ( bdd<VertexData, ArcData> lhs
                   , bdd<VertexData, ArcData> rhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> manipulator;
        return manipulator.apply(std::move(lhs), OR {}, std::move(rhs));
    }
    
    template<class VertexData, class ArcData>
    auto operator^ ( bdd<VertexData, ArcData> lhs
                   , bdd<VertexData, ArcData> rhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> manipulator;
        return manipulator.apply(std::move(lhs), XOR {}, std::move(rhs));
    }

    template<class VertexData, class ArcData>
    auto operator! ( bdd<VertexData, ArcData> lhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> manipulator;
        return manipulator.negate(std::move(lhs));
    }
}

#endif