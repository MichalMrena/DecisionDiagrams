#ifndef MIX_DD_BDD_MANAGER_HPP
#include "../bdd_manager.hpp"
#endif

namespace mix::dd
{
    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::negate
        (bdd_t const& d) -> bdd_t
    {
        return this->apply(d, XOR(), this->just_val(1));
    }
}