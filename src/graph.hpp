#ifndef MIX_DD_GRAPH
#define MIX_DD_GRAPH

#include <string>
#include <array>

namespace mix::dd
{
    template<class VertexData, class ArcData, size_t N = 2>
    struct graph
    {
        struct arc;

        struct vertex
        {
            std::string label;
            VertexData data;
            std::array<arc*, N> forwardStar;
        };

        struct arc
        {
            ArcData data;
            vertex* target;
        };
    };
}

#endif