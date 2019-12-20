#ifndef _MIX_DD_GRAPH_
#define _MIX_DD_GRAPH_

#include <array>
#include <utility>
#include "typedefs.hpp"
#include "utils/hash.hpp"

namespace mix::dd
{
    struct empty {};

    template<class VertexData, class ArcData, size_t N>
    struct vertex;

    template<class VertexData, class ArcData, size_t N>
    struct arc
    {
        using vertex_t = vertex<VertexData, ArcData, N>;

        ArcData   data;
        vertex_t* target;

        arc(vertex_t* const pTarget = nullptr);
    };

    template<class VertexData, class ArcData, size_t N>
    struct vertex
    {
        using forward_star_arr = std::array< arc<VertexData, ArcData, N>, N >;

        id_t       id;
        index_t    index;
        VertexData data;
        bool       mark;
        forward_star_arr forwardStar;

        vertex( const id_t pId
              , const index_t pIndex);
        
        vertex( const id_t pId
              , const index_t pIndex
              , forward_star_arr pForwardStar);

        auto is_leaf () const -> bool;
    };

    /* partial specialisation with no data */
    template<size_t N>
    struct arc<empty, empty, N>
    {
        using vertex_t = vertex<empty, empty, N>;

        vertex_t* target;

        arc(vertex_t* const pTarget = nullptr);
    };

    /* partial specialisation with no data */
    template<size_t N>
    struct vertex<empty, empty, N>
    {
        using forward_star_arr = std::array< arc<empty, empty, N>, N >;

        id_t    id;
        index_t index;
        bool    mark;
        forward_star_arr forwardStar;

        vertex( const id_t pId
              , const index_t pIndex);
        
        vertex( const id_t pId
              , const index_t pIndex
              , forward_star_arr pForwardStar);

        auto is_leaf () const -> bool;
    };

    template<class VertexData, class ArcData, size_t N>
    using vertex_pair = std::pair< const vertex<VertexData, ArcData, N>*
                                 , const vertex<VertexData, ArcData, N>* >;
    
    template<class VertexData, class ArcData, size_t N>
    struct vertex_pair_hash
    {
        auto operator() (const vertex_pair<VertexData, ArcData, N>& key) const -> size_t;
    };

    /* arc constructor */
    template<class VertexData, class ArcData, size_t N>
    arc<VertexData, ArcData, N>::arc(vertex_t* const pTarget) :
        target {pTarget}
    {
    }

    /* specialised arc constructor */
    template<size_t N>
    arc<empty, empty, N>::arc(vertex_t* const pTarget) :
        target {pTarget}
    {
    }

    /* vertex first constructor */
    template<class VertexData, class ArcData, size_t N>
    vertex<VertexData, ArcData, N>::vertex( const id_t pId
                                          , const index_t pIndex) :
        id          {pId}
      , index       {pIndex}
      , mark        {false}
      , forwardStar {}
    {
    }

    /* specialised vertex first constructor */
    template<size_t N>
    vertex<empty, empty, N>::vertex( const id_t pId
                                   , const index_t pIndex) :
        id          {pId}
      , index       {pIndex}
      , mark        {false}
      , forwardStar {}
    {
    }

    /* vertex second constructor */
    template<class VertexData, class ArcData, size_t N>
    vertex<VertexData, ArcData, N>::vertex( const id_t pId
                                          , const index_t pIndex
                                          , forward_star_arr pForwardStar) :
        id          {pId}
      , index       {pIndex}
      , mark        {false}
      , forwardStar {pForwardStar}
    {
    }

    /* specialised vertex second constructor */
    template<size_t N>
    vertex<empty, empty, N>::vertex( const id_t pId
                                   , const index_t pIndex
                                   , forward_star_arr pForwardStar) :
        id          {pId}
      , index       {pIndex}
      , mark        {false}
      , forwardStar {pForwardStar}
    {
    }

    /* vertex::is_leaf */
    template<class VertexData, class ArcData, size_t N>
    auto vertex<VertexData, ArcData, N>::is_leaf () const -> bool
    {
        return nullptr == this->forwardStar[0].target;
    }

    /* specialised vertex::is_leaf */
    template<size_t N>
    auto vertex<empty, empty, N>::is_leaf () const -> bool
    {
        return nullptr == this->forwardStar[0].target;
    }


    template<class VertexData, class ArcData, size_t N>
    auto vertex_pair_hash<VertexData, ArcData, N>::operator() 
        (const vertex_pair<VertexData, ArcData, N>& key) const -> size_t
    {
        using vertex_t = vertex<VertexData, ArcData, N>;
        size_t seed {0};

        // utils::boost::hash_combine<vertex*, utils::pointer_hash<vertex>>(seed, key.negative);
        // utils::boost::hash_combine<vertex*, utils::pointer_hash<vertex>>(seed, key.positive);

        utils::boost::hash_combine<const vertex_t*, std::hash<const vertex_t*>>(seed, key.first);
        utils::boost::hash_combine<const vertex_t*, std::hash<const vertex_t*>>(seed, key.second);

        return seed;
    }
}

#endif