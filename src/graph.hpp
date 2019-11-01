#ifndef MIX_DD_GRAPH
#define MIX_DD_GRAPH

#include <string>
#include <array>
#include "typedefs.hpp"

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
}

#endif