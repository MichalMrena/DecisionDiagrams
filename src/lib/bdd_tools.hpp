#ifndef MIX_DD_BDD_TOOLS_
#define MIX_DD_BDD_TOOLS_

#include "diagrams/bdd_creator.hpp"
#include "diagrams/bdd_manipulator.hpp"
#include "diagrams/bdd_reliability.hpp"
#include "diagrams/truth_vector.hpp"
#include "diagrams/pla_file.hpp"
#include "utils/recycling_pool.hpp"
#include "utils/pool_allocator.hpp"

namespace mix::dd
{   
    template<class VertexData = double, class ArcData = void>
    class bdd_tools
    {
    public:
        using vertex_t      = vertex<VertexData, ArcData, 2>;
        using pool_t        = utils::recycling_pool<vertex_t>;
        using allocator_t   = utils::pool_allocator<utils::recycling_pool<vertex_t>>;
        using creator_t     = bdd_creator<VertexData, ArcData, allocator_t>;
        using manipulator_t = bdd_manipulator<VertexData, ArcData, allocator_t>;
        using reliability_t = bdd_reliability<VertexData, ArcData, allocator_t>;

    public:
        auto creator     () -> creator_t;
        auto manipulator () -> manipulator_t;
        auto reliability () -> reliability_t;

    private:
        pool_t pool_;
    };

    template<class VertexData, class ArcData>
    auto bdd_tools<VertexData, ArcData>::creator
        () -> creator_t
    {
        return creator_t {allocator_t {pool_}};
    }

    template<class VertexData, class ArcData>
    auto bdd_tools<VertexData, ArcData>::manipulator
        () -> manipulator_t
    {
        return manipulator_t {allocator_t {pool_}};
    }

    template<class VertexData, class ArcData>
    auto bdd_tools<VertexData, ArcData>::reliability
        () -> reliability_t
    {
        return reliability_t {allocator_t {pool_}};
    }
}

#endif