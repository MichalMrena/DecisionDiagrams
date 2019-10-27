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
            const std::string label;
            const size_t level;
            VertexData data;
            std::array<arc, N> forwardStar;
            bool mark;

            vertex(const std::string pLabel
                 , const size_t pLevel);
            
            vertex(const std::string pLabel
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
    graph<VertexData, ArcData, N>::vertex::vertex(const std::string pLabel
                                                , const size_t pLevel) :
        label       {std::move(pLabel)}
      , level       {pLevel}
      , forwardStar {}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    graph<VertexData, ArcData, N>::vertex::vertex(const std::string pLabel
                                                , const size_t pLevel
                                                , std::array<arc, N> pForwardStar) :
        label       {std::move(pLabel)}
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