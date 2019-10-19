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

            explicit vertex(std::string pLabel);
            vertex(std::string pLabel, std::array<arc*, N> pForwardStar);
        };

        struct arc
        {
            ArcData data;
            vertex* target;

            explicit arc(vertex* pTarget);
        };
    };

    template<class VertexData, class ArcData, size_t N>
    graph<VertexData, ArcData, N>::vertex::vertex(std::string pLabel) :
        label {pLabel}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    graph<VertexData, ArcData, N>::vertex::vertex(std::string pLabel, std::array<arc*, N> pForwardStar) :
        label {pLabel}
      , forwardStar {pForwardStar}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    graph<VertexData, ArcData, N>::arc::arc(vertex* pTarget) :
        target {pTarget}
    {
    }
}

#endif