#ifndef MIX_DD_BDD_MANAGER_HPP
#define MIX_DD_BDD_MANAGER_HPP

#include "mdd_manager.hpp"
#include "diagrams/bdd_creator.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData>
    class bdd_manager : public mdd_manager<VertexData, ArcData, 2>
    {
    public:
        using base      = mdd_manager<VertexData, ArcData, 2>;
        using creator_t = bdd_creator<VertexData, ArcData>;

    public:
        bdd_manager (std::size_t const varCount);

        auto creator () -> creator_t;
    };

    template<class VertexData, class ArcData>
    bdd_manager<VertexData, ArcData>::bdd_manager
        (std::size_t const varCount) :
        base {varCount}
    {
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::creator
        () -> creator_t
    {
        return creator_t {&(base::manager_), this->manipulator()};
    }
}

#endif