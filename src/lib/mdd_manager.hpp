#ifndef MIX_DD_MDD_MANAGER_HPP
#define MIX_DD_MDD_MANAGER_HPP

#include "diagrams/vertex_manager.hpp"
#include "diagrams/mdd_creator.hpp"
#include "diagrams/mdd_tools.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    class mdd_manager
    {
    public:
        using creator_t = mdd_creator<VertexData, ArcData, P>;
        using tools_t   = mdd_tools<VertexData, ArcData, P>;

    public:
        mdd_manager (std::size_t const varCount);

        auto creator () -> creator_t;
        auto tools   () -> tools_t;

    private:
        using manager_t = vertex_manager<VertexData, ArcData, P>;

    private:
        manager_t manager_;
    };

    template<class VertexData, class ArcData, std::size_t P>
    mdd_manager<VertexData, ArcData, P>::mdd_manager
        (std::size_t const varCount) :
        manager_ {varCount}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::creator
        () -> creator_t
    {
        return creator_t {&manager_};
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::tools
        () -> tools_t
    {
        return tools_t {&manager_};
    }
}

#endif