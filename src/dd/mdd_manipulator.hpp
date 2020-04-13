#ifndef MIX_DD_MDD_MANIPULATOR_
#define MIX_DD_MDD_MANIPULATOR_

#include <algorithm>
#include <iterator>
#include <functional>
#include <utility>
#include "dd_manipulator_base.hpp"
#include "mdd.hpp"
#include "../utils/hash.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData, size_t P>
    class mdd_manipulator : public dd_manipulator_base<VertexData, ArcData, P>
    {
    public:
        using mdd_t    = mdd<VertexData, ArcData, P>;
        using vertex_t = typename mdd_t::vertex_t;
        using arc_t    = typename mdd_t::arc_t;
        using log_t    = typename mdd_t::log_t;

    public:
        virtual ~mdd_manipulator () = default;
        mdd_manipulator ();

    public:
        template<class BinOp> auto apply (mdd_t&&      d1, BinOp op, mdd_t&&      d2) -> mdd_t;
        template<class BinOp> auto apply (const mdd_t& d1, BinOp op, mdd_t&&      d2) -> mdd_t;
        template<class BinOp> auto apply (mdd_t&&      d1, BinOp op, const mdd_t& d2) -> mdd_t;
        template<class BinOp> auto apply (const mdd_t& d1, BinOp op, const mdd_t& d2) -> mdd_t;

        auto reduce (mdd_t&  diagram) -> mdd_t&;
        auto reduce (mdd_t&& diagram) -> mdd_t;

    private:
        using leaf_val_map       = std::map<const vertex_t*, log_t>;
        using val_leaf_arr       = std::array<vertex_t*, P>;
        using son_arr            = typename vertex_t::forward_star_arr;
        using recursion_key_t    = std::pair<const vertex_t*, const vertex_t*>;
        using recursion_memo_map = std::unordered_map<const recursion_key_t, vertex_t*, utils::tuple_hash_t<const recursion_key_t>>;
        using in_graph_key_t     = std::array<index_t, P + 1>;
        using in_graph_memo_map  = std::unordered_map<const in_graph_key_t, vertex_t*, utils::tuple_hash_t<const in_graph_key_t>>;

    private:
        template<class BinOp>
        auto apply_step ( const vertex_t* const v1
                        , BinOp                 op
                        , const vertex_t* const v2 ) -> vertex_t*;

        auto terminal_vertex ( const log_t    val )  -> vertex_t*;
        auto internal_vertex ( const index_t  index
                             , const son_arr& sons ) -> vertex_t*;

        auto index1 (const vertex_t* const v1) const -> index_t;
        auto index2 (const vertex_t* const v2) const -> index_t;
        auto value1 (const vertex_t* const v1) const -> log_t;
        auto value2 (const vertex_t* const v2) const -> log_t;

        auto leaf_index () const   -> index_t;
        auto recycle    (mdd_t& d) -> void;
        auto reset      ()         -> void;

    private:
        leaf_val_map       leafToVal_;
        val_leaf_arr       valToLeaf_;
        recursion_memo_map recursionMemo_;
        in_graph_memo_map  inGraphMemo_;
        id_t               nextId_;
        const mdd_t*       diagram1_;
        const mdd_t*       diagram2_;
    };

    template<class VertexData, class ArcData, size_t P>
    mdd_manipulator<VertexData, ArcData, P>::mdd_manipulator () :
        valToLeaf_ {{nullptr}}
      , nextId_    {0}
      , diagram1_  {nullptr}
      , diagram2_  {nullptr}
    {
    }

    template<class VertexData, class ArcData, size_t P>
    template<class BinOp>
    auto mdd_manipulator<VertexData, ArcData, P>::apply
        (mdd_t&& d1, BinOp op, mdd_t&& d2) -> mdd_t
    {
        auto newDiagram = this->apply(d1, op, d2);
        this->recycle(d1);
        this->recycle(d2);
        return newDiagram;
    }
    
    template<class VertexData, class ArcData, size_t P>
    template<class BinOp>
    auto mdd_manipulator<VertexData, ArcData, P>::apply
        (mdd_t&& d1, BinOp op, const mdd_t& d2) -> mdd_t
    {
        auto newDiagram = this->apply(d1, op, d2);
        this->recycle(d1);
        return newDiagram;
    }
    
    template<class VertexData, class ArcData, size_t P>
    template<class BinOp>
    auto mdd_manipulator<VertexData, ArcData, P>::apply
        (const mdd_t& d1, BinOp op, mdd_t&& d2) -> mdd_t
    {
        auto newDiagram = this->apply(d1, op, d2);
        this->recycle(d2);
        return newDiagram;
    }

    template<class VertexData, class ArcData, size_t P>
    template<class BinOp>
    auto mdd_manipulator<VertexData, ArcData, P>::apply
        (const mdd_t& d1, BinOp op, const mdd_t& d2) -> mdd_t
    {
        diagram1_ = &d1;
        diagram2_ = &d2;

        auto newDiagram = mdd_t { this->apply_step(d1.root_, op, d2.root_)
                                , std::max(d1.variableCount_, d2.variableCount_)
                                , std::move(leafToVal_) };

        this->reset();

        return newDiagram;
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd_manipulator<VertexData, ArcData, P>::reduce
        (mdd_t&& diagram) -> mdd_t
    {
        return mdd_t {std::move(this->reduce(diagram))};
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd_manipulator<VertexData, ArcData, P>::reduce
        (mdd_t& diagram) -> mdd_t&
    {
        using vertex_key_t     = std::array<id_t, P>;
        auto const levels      = diagram.fill_levels();
        auto redundantVertices = std::vector<vertex_t*> {};
        auto newDiagramMap     = std::unordered_map<id_t, vertex_t*> {};
        auto nextId            = static_cast<id_t>(0);

        auto make_leaf_key = [](auto val)
        {
            auto arr = vertex_key_t {val};
            std::fill(std::begin(arr) + 1, std::end(arr), -1);
            return arr;
        };

        auto make_internal_key = [](auto v)
        {
            auto arr = vertex_key_t {};
            std::transform( std::begin(v->forwardStar), std::end(v->forwardStar), std::begin(arr)
                          , [](auto const& arc) { return arc.target->id; } );
            return arr;
        };

        for (auto i = levels.size(); i > 0;)
        {
            --i;
            auto keyedVertices = std::vector<std::pair<vertex_key_t, vertex_t*>> {};
            
            for (auto const u : levels[i])
            {
                if (diagram.is_leaf(u))
                {
                    keyedVertices.emplace_back(make_leaf_key(diagram.value(u)), u);
                }
                else if (this->is_redundant(u))
                {
                    u->id = u->son(0)->id;
                    redundantVertices.emplace_back(u);
                }
                else
                {
                    keyedVertices.emplace_back(make_internal_key(u), u);
                }
            }

            std::sort(keyedVertices.begin(), keyedVertices.end());

            auto oldKey = make_leaf_key(-1);

            for (auto& [key, u] : keyedVertices)
            {
                if (key == oldKey)
                {
                    u->id = nextId;
                    redundantVertices.emplace_back(u);
                    if (diagram.is_leaf(u))
                    {
                        diagram.leafToVal.erase(u);
                    }
                }
                else
                {
                    nextId++;
                    u->id = nextId;

                    newDiagramMap.emplace(nextId_, u);
                    
                    if (! diagram.is_leaf(u))
                    {
                        for (auto i = 0u; i < P; ++i)
                        {
                            u->son(i) = newDiagramMap.at(u->son(i)->id);
                        }
                    }

                    oldKey = key;
                }
            }
        }
        
        diagram.root_ = newDiagramMap.at(diagram.root_->id);

        for (auto const v : redundantVertices)
        {
            this->release_vertex(v);
        }

        return diagram;
    }

    template<class VertexData, class ArcData, size_t P>
    template<class BinOp>
    auto mdd_manipulator<VertexData, ArcData, P>::apply_step 
        (const vertex_t* const v1, BinOp op, const vertex_t* const v2) -> vertex_t*
    {
        auto const uit = recursionMemo_.find(recursion_key_t {v1, v2});
        if (uit != recursionMemo_.end())
        {
            return (*uit).second;
        }

        auto const val = op(this->value1(v1), this->value2(v2));
        auto       u   = static_cast<vertex_t*>(nullptr);

        if (val != X)
        {
            u = this->terminal_vertex(val);
        }
        else
        {
            auto       arcs  = son_arr {};
            auto const index = std::min(this->index1(v1), this->index2(v2));
            
            for (auto i = 0u; i < P; ++i)
            {
                auto const first  = this->index1(v1) == index ? v1->son(i) : v1;
                auto const second = this->index2(v2) == index ? v2->son(i) : v2;

                arcs[i].target = this->apply_step(first, op, second);
            }

            u = this->internal_vertex(index, arcs);
        }

        recursionMemo_.emplace(recursion_key_t {v1, v2}, u);

        return u;
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd_manipulator<VertexData, ArcData, P>::terminal_vertex
        (const log_t val) -> vertex_t*
    {
        if (! valToLeaf_.at(val))
        {
            valToLeaf_[val] = this->create_vertex(nextId_++, this->leaf_index());
            leafToVal_.emplace(valToLeaf_.at(val), val);
        }

        return valToLeaf_.at(val);
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd_manipulator<VertexData, ArcData, P>::internal_vertex
        (const index_t index, const son_arr& arcs) -> vertex_t*
    {
        if (std::all_of( std::begin(arcs), std::end(arcs)
                       , [fid = arcs.at(0).target->id] (auto&& arc) { return arc.target->id == fid; } ))
        {
            return arcs.at(0).target;
        }

        auto key = in_graph_key_t {index};
        std::transform( std::begin(arcs), std::end(arcs), std::begin(key) + 1
                      , [](auto&& a) { return a.target->id; } );

        auto const inGraphIt = inGraphMemo_.find(key);
        if (inGraphIt != inGraphMemo_.end())
        {
            return (*inGraphIt).second;
        }

        auto const newVertex = this->create_vertex(nextId_++, index, arcs);
        inGraphMemo_.emplace(key, newVertex);

        return newVertex;
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd_manipulator<VertexData, ArcData, P>::index1
        (const vertex_t* const v1) const -> index_t
    {
        return diagram1_->is_leaf(v1) ? this->leaf_index()
                                      : v1->index;
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd_manipulator<VertexData, ArcData, P>::index2
        (const vertex_t* const v2) const -> index_t
    {
        return diagram2_->is_leaf(v2) ? this->leaf_index()
                                      : v2->index;
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd_manipulator<VertexData, ArcData, P>::value1
        (const vertex_t* const v1)  const -> log_t
    {
        return diagram1_->value(v1);
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd_manipulator<VertexData, ArcData, P>::value2
        (const vertex_t* const v2)  const -> log_t
    {
        return diagram2_->value(v2);
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd_manipulator<VertexData, ArcData, P>::leaf_index
        () const -> index_t
    {
        return std::max( diagram1_->leaf_index()
                       , diagram2_->leaf_index() );
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd_manipulator<VertexData, ArcData, P>::recycle
        (mdd_t& d) -> void
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

    template<class VertexData, class ArcData, size_t P>
    auto mdd_manipulator<VertexData, ArcData, P>::reset
        () -> void
    {
        leafToVal_.clear();
        recursionMemo_.clear();
        inGraphMemo_.clear();
        
        std::fill(std::begin(valToLeaf_), std::end(valToLeaf_), nullptr);
        
        diagram1_  = nullptr;
        diagram2_  = nullptr;
        nextId_    = 0;
    }
}

#endif