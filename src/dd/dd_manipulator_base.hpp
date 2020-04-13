#ifndef _MIX_DD_MANIPULATOR_BASE_
#define _MIX_DD_MANIPULATOR_BASE_

#include <utility>
#include "graph.hpp"
#include "object_pool.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData, size_t N>
    class dd_manipulator_base
    {
    public:
        using vertex_t = vertex<VertexData, ArcData, N>;

    public:
        virtual ~dd_manipulator_base() = default;
        
    protected:
        template<class... Args>
        auto create_vertex  (Args&&... args)    -> vertex_t*;
        auto release_vertex (vertex_t* const v) -> void;

    private:
        object_pool<vertex_t> pool_;
    };

    template<class VertexData, class ArcData, size_t N>
    template<class... Args>
    auto dd_manipulator_base<VertexData, ArcData, N>::create_vertex    
        (Args&&... args) -> vertex_t*
    {
        return pool_.create_object(std::forward<Args>(args)...);
    }

    template<class VertexData, class ArcData, size_t N>
    auto dd_manipulator_base<VertexData, ArcData, N>::release_vertex
        (vertex_t* const v) -> void
    {
        pool_.release_object(v);
    }
}

#endif