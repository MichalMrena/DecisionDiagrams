#ifndef MIX_DD_GRAPH
#define MIX_DD_GRAPH

#include <cstdint>
#include <array>

namespace mix::dd
{
    using label_t = int32_t;

    template<class VertexData, class ArcData, size_t N = 2>
    struct graph
    {
        struct arc;

        struct vertex
        {
            label_t id;
            VertexData data;

            std::array<arc*, N> forwardStar;
        };

        struct arc
        {
            ArcData data;
            vertex * target;
        };

        std::array<vertex*, N> leafs {};

    };
}

#endif