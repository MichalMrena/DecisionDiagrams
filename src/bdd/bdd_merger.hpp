#ifndef MIX_DD_BDD_MERGER
#define MIX_DD_BDD_MERGER

#include <unordered_map>
#include <map>
#include <algorithm>
#include <tuple>
#include "bdd.hpp"
#include "bdd_reducer.hpp"
#include "bdd_creator.hpp"
#include "operators.hpp"
#include "../dd/object_pool.hpp"
#include "../utils/hash.hpp"

#include "../utils/stats.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData>
    class bdd_merger
    {
    public:
        static utils::averager avg;

    private:
        using bdd_t                 = bdd<VertexData, ArcData>;
        using bdd_reducer_t         = bdd_reducer<VertexData, ArcData>;
        using bdd_creator_t         = bdd_creator<VertexData, ArcData>;
        using vertex_t              = typename bdd_t::vertex_t;
        using arc_t                 = typename bdd_t::arc_t;
        using vertex_pair_t         = std::pair<const vertex_t*, const vertex_t*>;
        using vertex_pair_hash_t    = utils::pair_hash<const vertex_t*, const vertex_t*>;
        using yet_in_triplet_t      = std::tuple<index_t, id_t, id_t>;
        using yet_in_triplet_hash_t = utils::tuple_hash<index_t, id_t, id_t>;

    private:
        std::unordered_map<vertex_pair_t, vertex_t*, vertex_pair_hash_t> recursionMemo;
        std::unordered_map<const yet_in_triplet_t, vertex_t*, yet_in_triplet_hash_t> inGraphMemo;
        std::map<const vertex_t*, bool_t> leafToVal;
        std::array<vertex_t*, 2> valToLeaf {nullptr, nullptr};

        const bdd_t* diagram1 {nullptr};
        const bdd_t* diagram2 {nullptr};

        id_t nextId {0};

        object_pool<vertex_t> vertexPool;

    public:
        template<class BinaryBoolOperator>
        auto merge ( const bdd_t& d1
                   , const bdd_t& d2
                   , BinaryBoolOperator op ) -> bdd_t;

        template<class BinaryBoolOperator>
        auto merge_reduced ( const bdd_t& d1
                           , const bdd_t& d2
                           , BinaryBoolOperator op ) -> bdd_t;

        template<class BinaryBoolOperator>
        auto merge_unordered ( bdd_t& d1
                             , bdd_t& d2
                             , BinaryBoolOperator op ) -> bdd_t;

        // TODO this is a way to go, params by value so you can choose move or copy construct
        template<class BinaryBoolOperator>
        auto merge_recycling ( bdd_t d1
                             , bdd_t d2
                             , BinaryBoolOperator op ) -> bdd_t;

    private:
        template<class BinaryBoolOperator>
        auto merge_internal ( const vertex_t* const v1
                            , const vertex_t* const v2
                            , BinaryBoolOperator op ) -> vertex_t*;

        template<class BinaryBoolOperator>
        auto merge_internal_reduced ( const vertex_t* const v1
                                    , const vertex_t* const v2
                                    , BinaryBoolOperator op ) -> vertex_t*;

        template<class BinaryBoolOperator>
        auto merge_internal_recycling ( const vertex_t* const v1
                                      , const vertex_t* const v2
                                      , BinaryBoolOperator op ) -> vertex_t*;

        auto leaf_index () const -> index_t;

        auto index1 (const vertex_t* const v1) const -> index_t;
        auto index2 (const vertex_t* const v2) const -> index_t;
        auto value1 (const vertex_t* const v1) const -> bool_t;
        auto value2 (const vertex_t* const v2) const -> bool_t;

        auto terminal_vertex (const bool_t val) -> vertex_t*;
        auto internal_vertex ( const index_t index
                             , vertex_t* const low
                             , vertex_t* const high ) -> vertex_t*;

        auto terminal_vertex_recycled (const bool_t val) -> vertex_t*;
        auto internal_vertex_recycled ( const index_t index
                                      , vertex_t* const low
                                      , vertex_t* const high ) -> vertex_t*;

        auto recycle (bdd_t& d) -> void;

        auto reset () -> void;

        template<class BinaryBoolOperator>
        static auto merge_internal_unordered ( bdd_t& d1, bdd_t& d2
                                             , BinaryBoolOperator op ) -> bdd_t;

        static auto absorbing_leaf ( vertex_t* const trueLeaf
                                   , vertex_t* const falseLeaf
                                   , AND ) -> vertex_t*;

        static auto absorbing_leaf ( vertex_t* const trueLeaf
                                   , vertex_t* const falseLeaf
                                   , OR ) -> vertex_t*;
        
        static auto is_just_const   (const bdd_t& d)    -> bool;
        static auto root_val        (const bdd_t& d)    -> bool_t;
        static auto share_variables ( const bdd_t& d1
                                    , const bdd_t& d2 ) -> bool;
    };

    template<class VertexData, class ArcData>
    utils::averager bdd_merger<VertexData, ArcData>::avg = utils::averager {};

    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_merger<VertexData, ArcData>::merge
        (const bdd_t& d1, const bdd_t& d2, BinaryBoolOperator op) -> bdd_t
    {
        this->diagram1 = &d1;
        this->diagram2 = &d2;

        bdd_merger::avg.add_value(d1.vertex_count());
        bdd_merger::avg.add_value(d2.vertex_count());

        vertex_t* newRoot 
        {
            this->merge_internal(d1.root, d2.root, op)
        };

        bdd_t newDiagram 
        {
            newRoot
          , std::max(d1.variableCount, d2.variableCount)
          , std::move(this->leafToVal)
        };

        this->reset();
        
        bdd_reducer_t reducer;
        reducer.reduce(newDiagram);

        // TODO možno ani neni treba explicitný move RVO by to mohlo vyriešiť,
        // najlepšie vyskúšať
        return bdd_t {std::move(newDiagram)};
    }

    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_merger<VertexData, ArcData>::merge_reduced
        (const bdd_t& d1, const bdd_t& d2, BinaryBoolOperator op) -> bdd_t
    {
        this->diagram1 = &d1;
        this->diagram2 = &d2;

        bdd_merger::avg.add_value(d1.vertex_count());
        bdd_merger::avg.add_value(d2.vertex_count());

        bdd_t newDiagram 
        {
            this->merge_internal_reduced(d1.root, d2.root, op)
          , std::max(d1.variableCount, d2.variableCount)
          , std::move(this->leafToVal)
        };

        this->reset();

        // TODO treba overit, ci sa nerobi move
        return newDiagram;
    }

    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_merger<VertexData, ArcData>::merge_unordered
        (bdd_t& d1, bdd_t& d2, BinaryBoolOperator op) -> bdd_t
    {
        const auto shareVariables {share_variables(d1, d2)};

        bdd_t newDiagram
        {
            merge_internal_unordered(d1, d2, op)
        };

        if (shareVariables)
        {
            // reduce unordered
        }

        return bdd_t {newDiagram};
    }

    
    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_merger<VertexData, ArcData>::merge_recycling
        (bdd_t d1, bdd_t d2, BinaryBoolOperator op) -> bdd_t
    {
        this->diagram1 = &d1;
        this->diagram2 = &d2;

        bdd_merger::avg.add_value(d1.vertex_count());
        bdd_merger::avg.add_value(d2.vertex_count());

        bdd_t newDiagram 
        {
            this->merge_internal_recycling(d1.root, d2.root, op)
          , std::max(d1.variableCount, d2.variableCount)
          , std::move(this->leafToVal)
        };

        this->reset();

        this->recycle(d1);
        this->recycle(d2);

        return newDiagram;
    }

    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_merger<VertexData, ArcData>::merge_internal 
        (const vertex_t* const v1, const vertex_t* const v2, BinaryBoolOperator op) -> vertex_t*
    {
        const auto uit {this->recursionMemo.find(vertex_pair_t {v1, v2})};
        if (uit != this->recursionMemo.end())
        {
            return (*uit).second;
        }

        const auto val {op(this->value1(v1), this->value2(v2))};
        vertex_t* u    {nullptr};

        if (val != X)
        {
            u = new vertex_t {this->nextId++, this->leaf_index()};
            this->leafToVal.emplace(u, val);
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

            u = new vertex_t {
                this->nextId++
              , index
              , { arc_t {this->merge_internal(vlow1, vlow2, op)}
                , arc_t {this->merge_internal(vhigh1, vhigh2, op)} }
            };
        }

        this->recursionMemo.emplace(vertex_pair_t {v1, v2}, u);

        return u;
    }

    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_merger<VertexData, ArcData>::merge_internal_reduced 
        (const vertex_t* const v1, const vertex_t* const v2, BinaryBoolOperator op) -> vertex_t*
    {
        const auto uit {this->recursionMemo.find(vertex_pair_t {v1, v2})};
        if (uit != this->recursionMemo.end())
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
                                     , this->merge_internal_reduced(vlow1, vlow2, op)
                                     , this->merge_internal_reduced(vhigh1, vhigh2, op) );
        }

        this->recursionMemo.emplace(vertex_pair_t {v1, v2}, u);

        return u;
    }

    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_merger<VertexData, ArcData>::merge_internal_recycling
        (const vertex_t* const v1, const vertex_t* const v2, BinaryBoolOperator op) -> vertex_t*
    {
        const auto uit {this->recursionMemo.find(vertex_pair_t {v1, v2})};
        if (uit != this->recursionMemo.end())
        {
            return (*uit).second;
        }

        const auto val {op(this->value1(v1), this->value2(v2))};
        vertex_t* u    {nullptr};

        if (val != X)
        {
            u = this->terminal_vertex_recycled(val);
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

            u = this->internal_vertex_recycled( 
                index
              , this->merge_internal_recycling(vlow1, vlow2, op)
              , this->merge_internal_recycling(vhigh1, vhigh2, op) );
        }

        this->recursionMemo.emplace(vertex_pair_t {v1, v2}, u);

        return u;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::leaf_index
        () const -> index_t
    {
        return std::max( this->diagram1->leaf_index()
                       , this->diagram2->leaf_index() );
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::index1
        (const vertex_t* const v1) const -> index_t
    {
        return this->diagram1->is_leaf(v1) ? this->leaf_index()
                                           : v1->index;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::index2
        (const vertex_t* const v2) const -> index_t
    {
        return this->diagram2->is_leaf(v2) ? this->leaf_index()
                                           : v2->index;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::value1
        (const vertex_t* const v1)  const -> bool_t
    {
        return this->diagram1->value(v1);
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::value2
        (const vertex_t* const v2)  const -> bool_t
    {
        return this->diagram2->value(v2);
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::terminal_vertex
        (const bool_t val) -> vertex_t*
    {
        if (! this->valToLeaf[val])
        {
            this->valToLeaf[val] = new vertex_t {this->nextId++, this->leaf_index()};
            this->leafToVal.emplace(this->valToLeaf[val], val);
        }

        return this->valToLeaf[val];
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::terminal_vertex_recycled
        (const bool_t val) -> vertex_t*
    {
        if (! this->valToLeaf[val])
        {
            this->valToLeaf[val] = this->vertexPool.get_object(this->nextId++, this->leaf_index());
            this->leafToVal.emplace(this->valToLeaf[val], val);
        }

        return this->valToLeaf[val];
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::internal_vertex
        (const index_t index, vertex_t* const low, vertex_t* const high ) -> vertex_t*
    {
        if (low->id == high->id)
        {
            return low;
        }

        const auto key       {std::make_tuple(index, low->id, high->id)};
        const auto inGraphIt {this->inGraphMemo.find(key)};
        
        if (inGraphIt != inGraphMemo.end())
        {
            return (*inGraphIt).second;
        }

        const auto newVertex {new vertex_t 
        {
            this->nextId++
          , index
          , { arc_t {low}, arc_t {high} }
        }};

        this->inGraphMemo.emplace(key, newVertex);

        return newVertex;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::internal_vertex_recycled
        (const index_t index, vertex_t* const low, vertex_t* const high ) -> vertex_t*
    {
        using arr_t = typename vertex_t::forward_star_arr;

        if (low->id == high->id)
        {
            return low;
        }

        const auto key       {std::make_tuple(index, low->id, high->id)};
        const auto inGraphIt {this->inGraphMemo.find(key)};
        
        if (inGraphIt != inGraphMemo.end())
        {
            return (*inGraphIt).second;
        }

        const auto newVertex
        {
            this->vertexPool.get_object(this->nextId++, index, arr_t{arc_t {low}, arc_t {high}})
        };

        this->inGraphMemo.emplace(key, newVertex);

        return newVertex;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::recycle
        (bdd_t& d) -> void
    {
        d.traverse(d.root, [this](vertex_t* const v) 
        {
            this->vertexPool.put_object(v);
        });

        d.root = nullptr;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::reset
        () -> void
    {
        this->recursionMemo.clear();
        this->inGraphMemo.clear();
        this->leafToVal.clear();
        this->valToLeaf = {nullptr, nullptr};
        this->diagram1  = nullptr;
        this->diagram2  = nullptr;
        this->nextId    = 0;
    }

    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_merger<VertexData, ArcData>::merge_internal_unordered
        (bdd_t& d1, bdd_t& d2, BinaryBoolOperator op) -> bdd_t
    {
        // first just handle corner cases when diagrams are simple:
        const auto rootOpVal {op(root_val(d1), root_val(d2))};
        if (X != rootOpVal)
        {
            return bdd_creator_t::just_val(rootOpVal);
        }

        if (is_just_const(d1))
        {
            return bdd_t {std::move(d2)};
        }
        else if (is_just_const(d2))
        {
            return bdd_t {std::move(d1)};
        }

        // repoint pointers from the first diagram to the second one:
        std::vector<std::pair<vertex_t*, bool_t>> toAbsorbing;
        std::vector<std::pair<vertex_t*, bool_t>> toOtherRoot;

        auto repoint_leaf_ptr {[&toAbsorbing, &toOtherRoot, &d1, op]
            (vertex_t* const v, const bool_t val) mutable
        {
            const auto sonVal {d1.value(v->son(val))};

            if (sonVal == absorbing_val(op))
            {
                toAbsorbing.emplace_back(v, val);
            }
            else if (sonVal == neutral_val(op))
            {
                toOtherRoot.emplace_back(v, val);
            }
        }};

        d1.traverse(d1.root, [&repoint_leaf_ptr, &d1](vertex_t* const v)
        {
            if (! d1.is_leaf(v))
            {
                repoint_leaf_ptr(v, 0);
                repoint_leaf_ptr(v, 1);
            }
        });

        const auto absorbingLeaf 
        {
            absorbing_leaf(d2.true_leaf(), d2.false_leaf(), op)
        };

        for (auto& [v, son] : toAbsorbing)
        {
            v->son(son) = absorbingLeaf;
        }

        for (auto& [v, son] : toOtherRoot)
        {
            v->son(son) = d2.root;
        }

        // possibly correct marks:
        if (d1.root->mark != d2.root->mark)
        {
            d2.traverse(d2.root, [](auto) {});
        }

        // create new diagram and empty d1 and d2:
        bdd_t newDiagram
        {
            d1.root
          , std::max(d1.variableCount, d2.variableCount)
          , std::move(d2.leafToVal)
        };

        d1.root = nullptr;
        d2.root = nullptr;
        delete d1.true_leaf();
        delete d1.false_leaf();
        d1.leafToVal.clear();
        
        // set new ids in the new diagram:
        id_t newId {0};
        newDiagram.traverse(newDiagram.root, [&newId](vertex_t* const v)
        {
            v->id = newId++;
        });

        return newDiagram;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::absorbing_leaf
        ( vertex_t* const
        , vertex_t* const falseLeaf
        , AND ) -> vertex_t*
    {
        return falseLeaf;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::absorbing_leaf
        ( vertex_t* const trueLeaf
        , vertex_t* const
        , OR ) -> vertex_t*
    {
        return trueLeaf;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::is_just_const
        (const bdd_t& d) -> bool
    {
        return d.is_leaf(d.root);
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::root_val
        (const bdd_t& d) -> bool_t
    {
        return d.value(d.root);
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::share_variables
        (const bdd_t& d1, const bdd_t& d2) -> bool
    {
        const auto indices1 {d1.indices()};
        const auto indices2 {d2.indices()};

        std::vector<index_t> indicesIntersection;
        indicesIntersection.reserve(std::min(indices1.size(), indices2.size()));

        std::set_intersection( indices1.begin(), indices1.end()
                             , indices2.begin(), indices2.end() 
                             , std::inserter(indicesIntersection, indicesIntersection.begin()) );

        return ! indicesIntersection.empty();
    }
}

#endif