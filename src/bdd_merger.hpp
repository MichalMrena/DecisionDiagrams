#ifndef MIX_DD_BDD_MERGER
#define MIX_DD_BDD_MERGER

#include <unordered_map>
#include <map>
#include <algorithm>
#include "bdd.hpp"
#include "operators.hpp"

namespace mix::dd
{
    template<class VertexData = int8_t, class ArcData = int8_t>
    class bdd_merger
    {
    private:
        using bdd_t            = bdd<VertexData, ArcData>;
        using arc              = typename graph<VertexData, ArcData>::arc;
        using vertex           = typename graph<VertexData, ArcData>::vertex;
        using vertex_pair      = typename graph<VertexData, ArcData>::vertex_pair;
        using vertex_pair_hash = typename graph<VertexData, ArcData>::vertex_pair_hash;

    private:
        std::unordered_map<vertex_pair, vertex*, vertex_pair_hash> memo;
        std::map<const vertex*, log_val_t> leafToVal;

        const bdd_t* diagram1 {nullptr};
        const bdd_t* diagram2 {nullptr};

        id_t nextId {1};

    public:
        template<class BinaryBoolOperator>
        auto merge (const bdd_t& d1
                  , const bdd_t& d2
                  , BinaryBoolOperator op) -> bdd_t;

    private:
        template<class BinaryBoolOperator>
        auto merge_internal (const vertex* const v1
                           , const vertex* const v2
                           , BinaryBoolOperator op) -> vertex*;

        auto leaf_index () const -> size_t;

        auto index1 (const vertex* const v1) const -> size_t;
        auto index2 (const vertex* const v2) const -> size_t;
        auto value1 (const vertex* const v1) const -> log_val_t;
        auto value2 (const vertex* const v2) const -> log_val_t;

        auto reset () -> void;
    };

    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_merger<VertexData, ArcData>::merge
        (const bdd_t& d1, const bdd_t& d2, BinaryBoolOperator op) -> bdd_t
    {
        this->reset();

        this->diagram1 = &d1;
        this->diagram2 = &d2;

        vertex* newRoot {
            this->merge_internal(d1.root, d2.root, op)
        };

        return bdd_t {
            newRoot
          , this->leaf_index() - 1
          , std::move(this->leafToVal)
        };
    }

    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_merger<VertexData, ArcData>::merge_internal 
        (const vertex* const v1, const vertex* const v2, BinaryBoolOperator op) -> vertex*
    {
        auto uit {this->memo.find(vertex_pair {v1, v2})};
        if (uit != this->memo.end())
        {
            return (*uit).second;
        }

        log_val_t val {op(this->value1(v1), this->value2(v2))};

        vertex* u {nullptr};

        if (val != X)
        {
            u = new vertex {this->nextId++, this->leaf_index()};
            this->leafToVal[u] = val;
        }
        else
        {
            const vertex* vlow1  {nullptr};
            const vertex* vlow2  {nullptr};
            const vertex* vhigh1 {nullptr};
            const vertex* vhigh2 {nullptr};

            const size_t index {std::min(this->index1(v1), this->index2(v2))};
            if (this->index1(v1) == index)
            {
                vlow1  = bdd_t::low(v1);
                vhigh1 = bdd_t::high(v1);
            }
            else
            {
                vlow1  = v1;
                vhigh1 = v1;
            }

            if (this->index2(v2) == index)
            {
                vlow2  = bdd_t::low(v2);
                vhigh2 = bdd_t::high(v2);
            }
            else
            {
                vlow2  = v2;
                vhigh2 = v2;
            }

            u = new vertex {
                this->nextId++
              , index
              , { arc {this->merge_internal(vlow1, vlow2, op)}
                , arc {this->merge_internal(vhigh1, vhigh2, op)}}
            };
        }

        this->memo[vertex_pair {v1, v2}] = u;

        return u;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::leaf_index
        () const -> size_t
    {
        return 1 + std::max(this->diagram1->variableCount
                          , this->diagram2->variableCount);
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::index1
        (const vertex* const v1) const -> size_t
    {
        if (this->diagram1->is_leaf(v1))
        {
            return this->leaf_index();
        }
        
        return v1->level;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::index2
        (const vertex* const v2) const -> size_t
    {
        if (this->diagram2->is_leaf(v2))
        {
            return this->leaf_index();
        }
        
        return v2->level;
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::value1
        (const vertex* const v1)  const -> log_val_t
    {
        return this->diagram1->value(v1);
    }

    template<class VertexData, class ArcData>
    auto bdd_merger<VertexData, ArcData>::value2
        (const vertex* const v2)  const -> log_val_t
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
        this->nextId   = 1;
    }

    template<class VertexData, class ArcData>
    auto operator&& (const bdd<VertexData, ArcData>& lhs
                   , const bdd<VertexData, ArcData>& rhs) -> bdd<VertexData, ArcData>
    {
        bdd_merger<VertexData, ArcData> merger;
        return merger.merge(lhs, rhs, AND {});
    }
    
    template<class VertexData, class ArcData>
    auto operator|| (const bdd<VertexData, ArcData>& lhs
                   , const bdd<VertexData, ArcData>& rhs) -> bdd<VertexData, ArcData>
    {
        bdd_merger<VertexData, ArcData> merger;
        return merger.merge(lhs, rhs, OR {});
    }
    
    template<class VertexData, class ArcData>
    auto operator^ (const bdd<VertexData, ArcData>& lhs
                  , const bdd<VertexData, ArcData>& rhs) -> bdd<VertexData, ArcData>
    {

    }

    template<class VertexData, class ArcData>
    auto operator! (const bdd<VertexData, ArcData>& lhs) -> bdd<VertexData, ArcData>
    {

    }
}

#endif