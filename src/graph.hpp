#ifndef MIX_DD_GRAPH
#define MIX_DD_GRAPH

#include <string>
#include <array>
#include <initializer_list>

namespace mix::dd
{
    template<class VertexData, class ArcData, size_t N = 2>
    struct graph
    {
        struct arc;

        struct vertex
        {
            const std::string label;
            VertexData data;
            std::array<arc*, N> forwardStar; // TODO arc by nemuseli byť dynamicky, stačili by prázdne s null targetom
            size_t level;

            vertex(std::string pLabel
                 , size_t pLevel);
            
            vertex(std::string pLabel
                 , std::array<arc*, N> pForwardStar
                 , size_t pLevel);

            auto is_leaf () const -> bool;
        };

        struct arc
        {
            ArcData data;
            vertex* target;

            explicit arc(vertex* pTarget);
        };
    };

    template<class VertexData, class ArcData, size_t N>
    graph<VertexData, ArcData, N>::vertex::vertex(std::string pLabel
                                                , size_t pLevel) :
        label {pLabel}
      , level {pLevel}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    graph<VertexData, ArcData, N>::vertex::vertex(std::string pLabel
                                                , std::array<arc*, N> pForwardStar
                                                , size_t pLevel) :
        label {pLabel}
      , forwardStar {pForwardStar}
      , level       {pLevel}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    auto graph<VertexData, ArcData, N>::vertex::is_leaf () const -> bool
    {
        for (arc* a : this->forwardStar)
        {
            if (a)
            {
                return false;
            }
        }

        return true;
    }

    template<class VertexData, class ArcData, size_t N>
    graph<VertexData, ArcData, N>::arc::arc(vertex* pTarget) :
        target {pTarget}
    {
    }
}

#endif