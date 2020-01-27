#ifndef MIX_DD_BDD_MERGER
#define MIX_DD_BDD_MERGER

#include <unordered_map>
#include <map>
#include <algorithm>
#include "bdd.hpp"
#include "bdd_reducer.hpp"
#include "operators.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData>
    class bdd_merger
    {
    private:
        using bdd_t              = bdd<VertexData, ArcData>;
        using bdd_reducer_t      = bdd_reducer<VertexData, ArcData>;
        using vertex_t           = vertex<VertexData, ArcData, 2>;
        using arc_t              = arc<VertexData, ArcData, 2>;
        using vertex_pair_t      = vertex_pair<VertexData, ArcData, 2>;
        using vertex_pair_hash_t = vertex_pair_hash<VertexData, ArcData, 2>;

    private:
        std::unordered_map<vertex_pair_t, vertex_t*, vertex_pair_hash_t> memo;
        std::map<const vertex_t*, bool_t> leafToVal;

        const bdd_t* diagram1 {nullptr};
        const bdd_t* diagram2 {nullptr};

        id_t nextId {0};

    public:
        template<class BinaryBoolOperator>
        auto merge ( const bdd_t& d1
                   , const bdd_t& d2
                   , BinaryBoolOperator op ) -> bdd_t;

    private:
        template<class BinaryBoolOperator>
        auto merge_internal ( const vertex_t* const v1
                            , const vertex_t* const v2
                            , BinaryBoolOperator op) -> vertex_t*;

        auto leaf_index () const -> index_t;

        auto index1 (const vertex_t* const v1) const -> index_t;
        auto index2 (const vertex_t* const v2) const -> index_t;
        auto value1 (const vertex_t* const v1) const -> bool_t;
        auto value2 (const vertex_t* const v2) const -> bool_t;

        auto reset () -> void;
    };

    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_merger<VertexData, ArcData>::merge
        (const bdd_t& d1, const bdd_t& d2, BinaryBoolOperator op) -> bdd_t
    {
        this->diagram1 = &d1;
        this->diagram2 = &d2;

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

        return bdd_t {std::move(newDiagram)};
    }

    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_merger<VertexData, ArcData>::merge_internal 
        (const vertex_t* const v1, const vertex_t* const v2, BinaryBoolOperator op) -> vertex_t*
    {
        auto uit {this->memo.find(vertex_pair_t {v1, v2})};
        if (uit != this->memo.end())
        {
            return (*uit).second;
        }

        const bool_t val {op(this->value1(v1), this->value2(v2))};

        vertex_t* u {nullptr};

        if (val != X)
        {
            u = new vertex_t {this->nextId++, this->leaf_index()};
            this->leafToVal[u] = val;
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
                , arc_t {this->merge_internal(vhigh1, vhigh2, op)}}
            };
        }

        // TODO emplace
        this->memo[vertex_pair_t {v1, v2}] = u;

        return u;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::leaf_index
        () const -> index_t
    {
        return std::max( this->diagram1->leaf_index()
                       , this->diagram2->leaf_index());
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
    auto bdd_merger<VertexData, ArcData>::reset
        () -> void
    {
        this->memo.clear();
        this->leafToVal.clear();
        this->diagram1 = nullptr;
        this->diagram2 = nullptr;
        this->nextId   = 0;
    }
}

#endif