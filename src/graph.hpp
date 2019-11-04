#ifndef MIX_DD_GRAPH
#define MIX_DD_GRAPH

#include <array>
#include "typedefs.hpp"
#include "utils/hash.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData, size_t N = 2>
    struct graph
    {
        struct vertex;

        struct arc
        {
            ArcData data;
            vertex* target {nullptr};

            arc() = default;
            explicit arc(vertex* const pTarget);
        };

        struct vertex
        {
            const id_t   id;
            const size_t level; // TODO rename to index
            VertexData data;
            std::array<arc, N> forwardStar;
            bool mark {false};

            vertex(const id_t pId
                 , const size_t pLevel);
            
            vertex(const id_t pId
                 , const size_t pLevel
                 , std::array<arc, N> pForwardStar);

            auto is_leaf () const -> bool;
        };

        struct vertex_pair
        {
            vertex* first;
            vertex* second;

            auto operator== (const vertex_pair& rhs) const -> bool;
        };
        
        struct vertex_pair_hash
        {
            auto operator() (const vertex_pair& key) const -> size_t;
        };

    };

    template<class VertexData, class ArcData, size_t N>
    graph<VertexData, ArcData, N>::arc::arc(vertex* const pTarget) :
        target {pTarget}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    graph<VertexData, ArcData, N>::vertex::vertex(const id_t pId
                                                , const size_t pLevel) :
        id          {pId}
      , level       {pLevel}
      , forwardStar {}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    graph<VertexData, ArcData, N>::vertex::vertex(const id_t pId
                                                , const size_t pLevel
                                                , std::array<arc, N> pForwardStar) :
        id          {pId}
      , level       {pLevel}
      , forwardStar {pForwardStar}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    auto graph<VertexData, ArcData, N>::vertex::is_leaf () const -> bool
    {
        for (const arc& a : this->forwardStar)
        {
            if (a.target)
            {
                return false;
            }
        }

        return true;
    }

    template<class VertexData, class ArcData, size_t N>
    auto graph<VertexData, ArcData, N>::vertex_pair::operator== 
        (const vertex_pair& other) const -> bool
    {
        return this->first  == other.first
            && this->second == other.second;
    }

    template<class VertexData, class ArcData, size_t N>
    auto graph<VertexData, ArcData, N>::vertex_pair_hash::operator() 
        (const vertex_pair& key) const -> size_t
    {
        size_t seed {0};

        // utils::boost::hash_combine<vertex*, utils::pointer_hash<vertex>>(seed, key.negative);
        // utils::boost::hash_combine<vertex*, utils::pointer_hash<vertex>>(seed, key.positive);

        utils::boost::hash_combine<vertex*, std::hash<vertex*>>(seed, key.negative);
        utils::boost::hash_combine<vertex*, std::hash<vertex*>>(seed, key.positive);

        return seed;
    }
}

#endif