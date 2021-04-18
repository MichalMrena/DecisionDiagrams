#ifndef MIX_DD_BDD_MANAGER_HPP
#include "../bdd_manager.hpp"
#endif

#include "operators.hpp"

namespace teddy
{
    template<class VertexData, class ArcData>
    bdd_manager<VertexData, ArcData>::bdd_manager
        (std::size_t const varCount, std::size_t const vertexCount) :
        base {varCount, vertexCount}
    {
    }

    auto make_bdd_manager(std::size_t const varCount, std::size_t const vertexCount)
    {
        return bdd_manager<double, void>(varCount, vertexCount);
    }

    template<class VertexData, class ArcData>
    auto operator! (mdd<VertexData, ArcData, 2> const& lhs) -> mdd<VertexData, ArcData, 2>
    {
        auto& mref = mm_impl::m_ref<VertexData, ArcData, 2>();
        auto& bref = static_cast<bdd_manager<VertexData, ArcData>&>(mref);
        return bref.negate(lhs);
    }
}