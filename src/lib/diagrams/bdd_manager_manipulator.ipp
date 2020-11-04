#ifndef MIX_DD_BDD_MANAGER_HPP
#include "../bdd_manager.hpp"
#endif

namespace mix::dd
{
    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::satisfy_count
        (bdd_t& d) -> std::size_t
    {
        return base::satisfy_count(d, 1);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::negate
        (bdd_t const& d) -> bdd_t
    {
        return this->transform(T_NEGATE, d, [this](auto const v)
        {
            auto& m = this->vertexManager_;
            return 0 == m.get_value(v) ? m.terminal_vertex(1) :
                   1 == m.get_value(v) ? m.terminal_vertex(0) : v;
        });
    }
}