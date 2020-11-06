#ifndef MIX_DD_BDD_MANAGER_HPP
#include "../bdd_manager.hpp"
#endif

namespace mix::dd
{
    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::negate
        (bdd_t const& d) -> bdd_t
    {
        return this->transform_terminal(d, [](auto const val)
        {
            return 0 == val ? 1 :
                   1 == val ? 0 : val;
        });
    }
}