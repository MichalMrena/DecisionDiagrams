#ifndef MIX_DD_MDD_TOOLS_HPP
#define MIX_DD_MDD_TOOLS_HPP

#include "diagrams/mdd_creator.hpp"
#include "diagrams/vertex_manager.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    class mdd_tools
    {
    public:
        mdd_tools (std::size_t const varCount);

    private:
        using manager_t = vertex_manager<VertexData, ArcData, P>;

    private:
        manager_t manager_;
    };

    template<class VertexData, class ArcData, std::size_t P>
    mdd_tools<VertexData, ArcData, P>::mdd_tools
        (std::size_t const varCount) :
        manager_ {varCount}
    {
    }
}

#endif