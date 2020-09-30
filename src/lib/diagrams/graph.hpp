#ifndef MIX_DD_GRAPH_
#define MIX_DD_GRAPH_

#include "typedefs.hpp"

#include <array>

namespace mix::dd
{
    /** vertex forward celaration */
    template<class VertexData, class ArcData, std::size_t P>
    struct vertex;

    /** arc base */
    template<class VertexData, class ArcData, std::size_t P>
    struct arc_base
    {
        using vertex_t = vertex<VertexData, ArcData, P>;
        vertex_t* target {nullptr};
    };

    /** normal arc */
    template<class VertexData, class ArcData, std::size_t P>
    struct arc : public arc_base<VertexData, ArcData, P>
    {
        using vertex_t = typename arc_base<VertexData, ArcData, P>::vertex_t;
        ArcData data;
        arc () = default;
        arc (vertex_t* const pTarget);
    };

    /** arc with no data */
    template<class VertexData, std::size_t P>
    struct arc<VertexData, void, P> : public arc_base<VertexData, void, P>
    {
        using vertex_t = typename arc_base<VertexData, void, P>::vertex_t;
        arc () = default;
        arc (vertex_t* const pTarget);
    };

    /** vertex base */
    template<class VertexData, class ArcData, std::size_t P>
    struct vertex_base
    {
        using star_arr = std::array< arc<VertexData, ArcData, P>, P >;
        using log_t    = typename log_val_traits<P>::type;

        id_t    id;
        index_t index;
        bool    mark; // TODO mark could be highest bit of the id
        star_arr forwardStar;

        vertex_base( id_t    const pId
                   , index_t const pIndex
                   , bool    const mark = false );

        vertex_base( id_t     const pId
                   , index_t  const pIndex
                   , star_arr const pForwardStar
                   , bool     const mark = false );

        auto son (log_t const i) const -> vertex<VertexData, ArcData, P>*; 
        auto son (log_t const i)       -> vertex<VertexData, ArcData, P>*&; 
    };

    /** normal vertex */
    template<class VertexData, class ArcData, std::size_t P>
    struct vertex : public vertex_base<VertexData, ArcData, P>
    {
        using star_arr = typename vertex_base<VertexData, ArcData, P>::star_arr;
        using log_t    = typename log_val_traits<P>::type;

        VertexData data;

        vertex( id_t    const pId
              , index_t const pIndex
              , bool    const mark = false );

        vertex( id_t     const pId
              , index_t  const pIndex
              , star_arr const pForwardStar
              , bool     const mark = false );
    };

    /** vertex with no data */
    template<class ArcData, std::size_t P>
    struct vertex<void, ArcData, P> : public vertex_base<void, ArcData, P>
    {
        using star_arr = typename vertex_base<void, ArcData, P>::star_arr;
        using log_t    = typename log_val_traits<P>::type;

        vertex( id_t    const pId
              , index_t const pIndex
              , bool    const mark = false );

        vertex( id_t     const pId
              , index_t  const pIndex
              , star_arr const pForwardStar
              , bool     const mark = false );
    };

// arc definitions:

    template<class VertexData, class ArcData, std::size_t P>
    arc<VertexData, ArcData, P>::arc
        (vertex_t* const pTarget) :
        arc_base<VertexData, ArcData, P> {pTarget}
    {
    }

    template<class VertexData, std::size_t P>
    arc<VertexData, void, P>::arc
        (vertex_t* const pTarget) :
        arc_base<VertexData, void, P> {pTarget}
    {
    }

// vertex_base definitions:

    template<class VertexData, class ArcData, std::size_t P>
    vertex_base<VertexData, ArcData, P>::vertex_base
        ( id_t    const pId
        , index_t const pIndex
        , bool    const mark ) :
        id          {pId},
        index       {pIndex},
        mark        {mark},
        forwardStar {}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex_base<VertexData, ArcData, P>::vertex_base
        ( id_t     const pId
        , index_t  const pIndex
        , star_arr const pForwardStar
        , bool     const mark ) :
        id          {pId},
        index       {pIndex},
        mark        {mark},
        forwardStar {pForwardStar}
    {
    }
    
    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::son
        (log_t const i) const -> vertex<VertexData, ArcData, P>*
    {
        return this->forwardStar.at(i).target;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::son
        (log_t const i) -> vertex<VertexData, ArcData, P>*&
    {
        return this->forwardStar.at(i).target;
    }

// normal vertex definitions:

    template<class VertexData, class ArcData, std::size_t P>
    vertex<VertexData, ArcData, P>::vertex
        ( id_t    const pId
        , index_t const pIndex
        , bool    const mark ) :
        vertex_base<VertexData, ArcData, P> {pId, pIndex, mark}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex<VertexData, ArcData, P>::vertex
        ( id_t     const pId
        , index_t  const pIndex
        , star_arr const pForwardStar
        , bool     const mark ) :
        vertex_base<VertexData, ArcData, P> {pId, pIndex, pForwardStar, mark}
    {
    }

// specialized empty vertex definitions:

    template<class ArcData, std::size_t P>
    vertex<void, ArcData, P>::vertex
        ( id_t    const pId
        , index_t const pIndex
        , bool    const mark ) :
        vertex_base<void, ArcData, P> {pId, pIndex, mark}
    {
    }

    template<class ArcData, std::size_t P>
    vertex<void, ArcData, P>::vertex
        ( id_t     const pId
        , index_t  const pIndex
        , star_arr const pForwardStar
        , bool     const mark ) :
        vertex_base<void, ArcData, P> {pId, pIndex, pForwardStar, mark}
    {
    }
}

#endif