#ifndef _MIX_DD_GRAPH_
#define _MIX_DD_GRAPH_

#include <array>
#include <utility>
#include "typedefs.hpp"
#include "../utils/hash.hpp"

namespace mix::dd
{
    struct empty_t {};

    template<class VertexData, class ArcData, size_t N>
    struct vertex;

    /** arc base */
    template<class VertexData, class ArcData, size_t N>
    struct arc_base
    {
        using vertex_t = vertex<VertexData, ArcData, N>;

        vertex_t* target {nullptr};

        arc_base() = default;
        arc_base(vertex_t* const pTarget);
        arc_base(const arc_base& other);
    };

    /** normal arc */
    template<class VertexData, class ArcData, size_t N>
    struct arc : public arc_base<VertexData, ArcData, N>
    {
        using vertex_t = typename arc_base<VertexData, ArcData, N>::vertex_t;

        ArcData data;

        arc() = default;
        arc(vertex_t* const pTarget);
        arc(const arc& other);
    };

    /** arc with no data */
    template<class VertexData, size_t N>
    struct arc<VertexData, empty_t, N> : public arc_base<VertexData, empty_t, N>
    {
        using vertex_t = typename arc_base<VertexData, empty_t, N>::vertex_t;

        arc() = default;
        arc(vertex_t* const pTarget);
        arc(const arc& other);
    };

    /** vertex base */
    template<class VertexData, class ArcData, size_t N>
    struct vertex_base
    {
        using forward_star_arr = std::array< arc<VertexData, ArcData, N>, N >;

        id_t    id;
        index_t index;
        bool    mark;
        forward_star_arr forwardStar;

        vertex_base( const id_t pId
                   , const index_t pIndex);
        
        vertex_base( const id_t pId
                   , const index_t pIndex
                   , forward_star_arr pForwardStar);

        /**
            Copy constructor.
            Copies id, index, mark and arc data.
            Because single vertex / arc doesn't have an information
            about the full structure of the graph, this constructor 
            does NOT create any new vertices which means
            that the result is just a shallow copy.
            Deep copy must be ensured from outside.
        */
        vertex_base(const vertex_base& other);

        auto is_leaf () const -> bool;
        auto son     (const bool_t i) const -> vertex<VertexData, ArcData, N>*; 
        auto son     (const bool_t i)       -> vertex<VertexData, ArcData, N>*&; 
    };

    /** normal vertex */
    template<class VertexData, class ArcData, size_t N>
    struct vertex : public vertex_base<VertexData, ArcData, N>
    {
        using forward_star_arr = typename vertex_base<VertexData, ArcData, N>::forward_star_arr;

        VertexData data;

        vertex( const id_t pId
              , const index_t pIndex);
        
        vertex( const id_t pId
              , const index_t pIndex
              , forward_star_arr pForwardStar);

        /*
            Copy constructor.
            Copies data stored in the vertex and calls
            copy construtor of the base.
        */
        vertex(const vertex& other);
    };

    /** vertex with no data */
    template<class ArcData, size_t N>
    struct vertex<empty_t, ArcData, N> : public vertex_base<empty_t, ArcData, N>
    {
        using forward_star_arr = typename vertex_base<empty_t, ArcData, N>::forward_star_arr;

        vertex( const id_t pId
              , const index_t pIndex);
        
        vertex( const id_t pId
              , const index_t pIndex
              , forward_star_arr pForwardStar);

        /*
            Copy constructor.
            Just calls copy construtor of the base.
        */
        vertex(const vertex& other);
    };

    // /** pair of pointers to const vertices */
    // template<class VertexData, class ArcData, size_t N>
    // using vertex_pair = std::pair< const vertex<VertexData, ArcData, N>*
    //                              , const vertex<VertexData, ArcData, N>* >;
    
    // /** hash of the pair declared above */
    // template<class VertexData, class ArcData, size_t N>
    // struct vertex_pair_hash
    // {
    //     auto operator() (const vertex_pair<VertexData, ArcData, N>& key) const -> size_t;
    // };

// arc_base, arc definitions:

    template<class VertexData, class ArcData, size_t N>
    arc_base<VertexData, ArcData, N>::arc_base(vertex_t* const pTarget) :
        target {pTarget}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    arc_base<VertexData, ArcData, N>::arc_base(const arc_base& other) :
        target {other.target}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    arc<VertexData, ArcData, N>::arc(vertex_t* const pTarget) :
        arc_base<VertexData, ArcData, N> {pTarget}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    arc<VertexData, ArcData, N>::arc(const arc& other) :
        arc_base<VertexData, ArcData, N> {other}
      , data {other.data}
    {
    }

    template<class VertexData, size_t N>
    arc<VertexData, empty_t, N>::arc(vertex_t* const pTarget) :
        arc_base<VertexData, empty_t, N> {pTarget}
    {
    }

    template<class VertexData, size_t N>
    arc<VertexData, empty_t, N>::arc(const arc& other) :
        arc_base<VertexData, empty_t, N> {other}
    {
    }

// vertex_base definitions:

    template<class VertexData, class ArcData, size_t N>
    vertex_base<VertexData, ArcData, N>::vertex_base( const id_t pId
                                                    , const index_t pIndex) :
        id          {pId}
      , index       {pIndex}
      , mark        {false}
      , forwardStar {}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    vertex_base<VertexData, ArcData, N>::vertex_base( const id_t pId
                                                    , const index_t pIndex
                                                    , forward_star_arr pForwardStar) :
        id          {pId}
      , index       {pIndex}
      , mark        {false}
      , forwardStar {pForwardStar}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    vertex_base<VertexData, ArcData, N>::vertex_base(const vertex_base& other) :
        id          {other.id}
      , index       {other.index}
      , mark        {other.mark}
      , forwardStar {other.forwardStar}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    auto vertex_base<VertexData, ArcData, N>::is_leaf 
        () const -> bool
    {
        return nullptr == this->forwardStar[0].target;
    }
    
    template<class VertexData, class ArcData, size_t N>
    auto vertex_base<VertexData, ArcData, N>::son
        (const bool_t i) const -> vertex<VertexData, ArcData, N>*
    {
        return this->forwardStar[i].target;
    }

    template<class VertexData, class ArcData, size_t N>
    auto vertex_base<VertexData, ArcData, N>::son
        (const bool_t i) -> vertex<VertexData, ArcData, N>*&
    {
        return this->forwardStar[i].target;
    }
    
// vertex definitions:

    template<class VertexData, class ArcData, size_t N>
    vertex<VertexData, ArcData, N>::vertex( const id_t pId
                                          , const index_t pIndex) :
        vertex_base<VertexData, ArcData, N> {pId, pIndex}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    vertex<VertexData, ArcData, N>::vertex( const id_t pId
                                          , const index_t pIndex
                                          , forward_star_arr pForwardStar) :
        vertex_base<VertexData, ArcData, N> {pId, pIndex, pForwardStar}
    {
    }

    template<class VertexData, class ArcData, size_t N>
    vertex<VertexData, ArcData, N>::vertex(const vertex& other) :
        vertex_base<VertexData, ArcData, N> {other}
      , data        {other.data}
    {
    }

// specialized empty vertex definitions:

    template<class ArcData, size_t N>
    vertex<empty_t, ArcData, N>::vertex( const id_t pId
                                     , const index_t pIndex) :
        vertex_base<empty_t, ArcData, N> {pId, pIndex}
    {
    }

    template<class ArcData, size_t N>
    vertex<empty_t, ArcData, N>::vertex( const id_t pId
                                     , const index_t pIndex
                                     , forward_star_arr pForwardStar) :
        vertex_base<empty_t, ArcData, N> {pId, pIndex, pForwardStar}
    {
    }
    
    template<class ArcData, size_t N>
    vertex<empty_t, ArcData, N>::vertex(const vertex& other) :
        vertex_base<empty_t, ArcData, N> {other}
    {
    }

// vertex_pair_hash definitions:

    // template<class VertexData, class ArcData, size_t N>
    // auto vertex_pair_hash<VertexData, ArcData, N>::operator() 
    //     (const vertex_pair<VertexData, ArcData, N>& key) const -> size_t
    // {
    //     using vertex_t = vertex<VertexData, ArcData, N>;
    //     size_t seed {0};

    //     // utils::boost::hash_combine<vertex*, utils::pointer_hash<vertex>>(seed, key.negative);
    //     // utils::boost::hash_combine<vertex*, utils::pointer_hash<vertex>>(seed, key.positive);

    //     utils::boost::hash_combine<const vertex_t*, std::hash<const vertex_t*>>(seed, key.first);
    //     utils::boost::hash_combine<const vertex_t*, std::hash<const vertex_t*>>(seed, key.second);

    //     return seed;
    // }
}

#endif