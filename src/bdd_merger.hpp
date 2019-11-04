#ifndef MIX_DD_BDD_MERGER
#define MIX_DD_BDD_MERGER

#include <unordered_map>
#include "bdd.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData>
    class bdd_merger
    {
    private:
        using bdd              = bdd<VertexData, ArcData>;
        using vertex           = typename graph<VertexData, ArcData>::vertex;
        using vertex_pair      = typename graph<VertexData, ArcData>::vertex_pair;
        using vertex_pair_hash = typename graph<VertexData, ArcData>::vertex_pair_hash;
        using pair_memo        = std::unordered_map<vertex_pair, vertex*, vertex_pair_hash>;

    private:
        pair_memo memo;
        
    public:
        template<class BinaryBoolOperator>
        auto merge (const bdd& d1, const bdd& d2) -> bdd;

    private:
        

    };

    template<class VertexData, class ArcData>
    auto operator&& (const bdd<VertexData, ArcData>& lhs
                   , const bdd<VertexData, ArcData>& rhs) -> bdd<VertexData, ArcData>
    {

    }
    
    template<class VertexData, class ArcData>
    auto operator|| (const bdd<VertexData, ArcData>& lhs
                   , const bdd<VertexData, ArcData>& rhs) -> bdd<VertexData, ArcData>
    {

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