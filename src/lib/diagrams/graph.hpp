#ifndef MIX_DD_GRAPH_HPP
#define MIX_DD_GRAPH_HPP

#include "typedefs.hpp"

#include <array>
#include <cstddef>

namespace mix::dd
{
    /** vertex forward celaration */
    template<class VertexData, class ArcData, std::size_t P>
    class vertex;

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
    class vertex_base
    {
    public:
        using star_arr = std::array< arc<VertexData, ArcData, P>, P >;
        using log_t    = typename log_val_traits<P>::type;
        using vertex_t = vertex<VertexData, ArcData, P>;

    public:
        vertex_base( id_t    const id
                   , index_t const index
                   , bool    const mark = false );

        vertex_base( id_t     const id
                   , index_t  const index
                   , star_arr const forwardStar
                   , bool     const mark = false );

    public:
        auto get_id      () const        -> id_t;
        auto set_id      (id_t const id) -> void;
        auto set_son     (log_t const i, vertex_t* const son) -> void;
        auto get_son     (log_t const i) const -> vertex_t*;
        auto set_mark    (bool const mark)     -> void;
        auto get_mark    () const              -> bool;
        auto toggle_mark ()                    -> void;
        auto get_index   () const              -> index_t;

    private:
        static constexpr auto id_mask   () -> id_t;
        static constexpr auto mark_mask () -> id_t;
        static constexpr auto mark_mask (bool const mask) -> id_t;

    private:
        id_t     id_;
        index_t  index_;
        star_arr forwardStar_;
    };

    /** normal vertex */
    template<class VertexData, class ArcData, std::size_t P>
    class vertex : public vertex_base<VertexData, ArcData, P>
    {
    public:
        using star_arr = typename vertex_base<VertexData, ArcData, P>::star_arr;
        using log_t    = typename log_val_traits<P>::type;
        using base_t   = vertex_base<VertexData, ArcData, P>;

    public:
        VertexData data;

    public:
        vertex( id_t    const id
              , index_t const index
              , bool    const mark = false );

        vertex( id_t     const id
              , index_t  const index
              , star_arr const forwardStar
              , bool     const mark = false );
    };

    /** vertex with no data */
    template<class ArcData, std::size_t P>
    class vertex<void, ArcData, P> : public vertex_base<void, ArcData, P>
    {
    public:
        using star_arr = typename vertex_base<void, ArcData, P>::star_arr;
        using log_t    = typename log_val_traits<P>::type;
        using base_t   = vertex_base<void, ArcData, P>;

    public:
        vertex( id_t    const id
              , index_t const index
              , bool    const mark = false );

        vertex( id_t     const id
              , index_t  const index
              , star_arr const forwardStar
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
        ( id_t    const id
        , index_t const index
        , bool    const mark ) :
        vertex_base {id, index, {}, mark}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex_base<VertexData, ArcData, P>::vertex_base
        ( id_t     const id
        , index_t  const index
        , star_arr const forwardStar
        , bool     const mark ) :
        id_          {(id & id_mask()) | mark_mask(mark)},
        index_       {index},
        forwardStar_ {forwardStar}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_id
        () const -> id_t
    {
        return id_ & id_mask();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::set_id
        (id_t const id) -> void
    {
        id_ = (id & id_mask()) | mark_mask(this->get_mark());
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_son
        (log_t const i) const -> vertex_t*
    {
        return forwardStar_[i].target;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::set_son
        (log_t const i, vertex_t* const son) -> void
    {
        forwardStar_[i] = son;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_mark
        () const -> bool
    {
        return id_ & mark_mask();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::toggle_mark
        () -> void
    {
        id_ ^= mark_mask();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::set_mark
        (bool const mark) -> void
    {
        id_ |= mark_mask(mark);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_index
        () const -> index_t
    {
        return index_;
    }

    template<class VertexData, class ArcData, std::size_t P>
    constexpr auto vertex_base<VertexData, ArcData, P>::mark_mask
        () -> id_t
    {
        // 1 in the highest bit, rest 0s
        return mark_mask(true);
    }

    template<class VertexData, class ArcData, std::size_t P>
    constexpr auto vertex_base<VertexData, ArcData, P>::mark_mask
        (bool const mask) -> id_t
    {
        // 1 or 0 in the highest bit, rest 0s
        return (static_cast<id_t>(mask) << (8 * sizeof(id_t) - 1));
    }

    template<class VertexData, class ArcData, std::size_t P>
    constexpr auto vertex_base<VertexData, ArcData, P>::id_mask
        () -> id_t
    {
        // 0 in the highest bit, rest 1s
        return ~mark_mask();
    }

// normal vertex definitions:

    template<class VertexData, class ArcData, std::size_t P>
    vertex<VertexData, ArcData, P>::vertex
        ( id_t    const id
        , index_t const index
        , bool    const mark ) :
        base_t {id, index, mark}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex<VertexData, ArcData, P>::vertex
        ( id_t     const id
        , index_t  const index
        , star_arr const forwardStar
        , bool     const mark ) :
        base_t {id, index, forwardStar, mark}
    {
    }

// specialized empty vertex definitions:

    template<class ArcData, std::size_t P>
    vertex<void, ArcData, P>::vertex
        ( id_t    const id
        , index_t const index
        , bool    const mark ) :
        base_t {id, index, mark}
    {
    }

    template<class ArcData, std::size_t P>
    vertex<void, ArcData, P>::vertex
        ( id_t     const id
        , index_t  const index
        , star_arr const forwardStar
        , bool     const mark ) :
        base_t {id, index, forwardStar, mark}
    {
    }
}

#endif