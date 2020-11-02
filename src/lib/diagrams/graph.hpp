#ifndef MIX_DD_GRAPH_HPP
#define MIX_DD_GRAPH_HPP

#include "typedefs.hpp"
#include "../utils/more_algorithm.hpp"

#include <array>
#include <cstddef>

namespace mix::dd
{
    /** vertex forward celaration */
    template<class VertexData, class ArcData, std::size_t P>
    class vertex;

    /**
        @brief Arc base.
     */
    template<class VertexData, class ArcData, std::size_t P>
    struct arc_base
    {
        using vertex_t = vertex<VertexData, ArcData, P>;
        vertex_t* target {nullptr};
    };

    /**
        @brief Arc with data member variable.
     */
    template<class VertexData, class ArcData, std::size_t P>
    struct arc : public arc_base<VertexData, ArcData, P>
    {
        using vertex_t = typename arc_base<VertexData, ArcData, P>::vertex_t;
        ArcData data;
        arc () = default;
        arc (vertex_t* const pTarget);
    };

    /**
        @brief Arc with without data member variable.
     */
    template<class VertexData, std::size_t P>
    struct arc<VertexData, void, P> : public arc_base<VertexData, void, P>
    {
        using vertex_t = typename arc_base<VertexData, void, P>::vertex_t;
        arc () = default;
        arc (vertex_t* const pTarget);
    };

    /**
       @brief Vertex base.
     */
    template<class VertexData, class ArcData, std::size_t P>
    class vertex_base
    {
    public:
        using vertex_t = vertex<VertexData, ArcData, P>;
        using arc_t    = arc<VertexData, ArcData, P>;
        using arc_a    = std::array<arc_t, P>;
        using son_a    = std::array<vertex_t*, P>;
        using log_t    = typename log_val_traits<P>::type;

    public:
        vertex_base( index_t const  index );
        vertex_base( index_t const  index
                   , son_a   const& sons );

    public:
        auto get_son       (log_t const i) const -> vertex_t*;
        auto set_mark      (bool const mark)     -> void;
        auto get_mark      () const              -> bool;
        auto toggle_mark   ()                    -> void;
        auto get_index     () const              -> index_t;
        auto set_index     (index_t const index) -> void;
        auto get_ref_count () const -> std::size_t;
        auto inc_ref_count ()       -> void;
        auto dec_ref_count ()       -> void;

    private:
        bool        mark_;
        index_t     index_;
        arc_a       forwardStar_;
        std::size_t refCount_;
    };

    /**
        @brief Vertex with data member variable.
     */
    template<class VertexData, class ArcData, std::size_t P>
    class vertex : public vertex_base<VertexData, ArcData, P>
    {
    public:
        using son_a  = typename vertex_base<VertexData, ArcData, P>::son_a;
        using log_t  = typename log_val_traits<P>::type;
        using base_t = vertex_base<VertexData, ArcData, P>;

    public:
        VertexData data;

    public:
        vertex( index_t const  index );
        vertex( index_t const  index
              , son_a   const& sons );
    };

    /**
        @brief Specialized vertex without data member variable.
     */
    template<class ArcData, std::size_t P>
    class vertex<void, ArcData, P> : public vertex_base<void, ArcData, P>
    {
    public:
        using son_a  = typename vertex_base<void, ArcData, P>::son_a;
        using base_t = vertex_base<void, ArcData, P>;

    public:
        vertex( index_t const  index );
        vertex( index_t const  index
              , son_a   const& sons );
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
        (index_t const index) :
        vertex_base {index, {}}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex_base<VertexData, ArcData, P>::vertex_base
        ( index_t const  index
        , son_a   const& sons ) :
        mark_        {false},
        index_       {index},
        forwardStar_ {utils::map_to_array(sons, [](auto const v) { return arc_t {v}; })},
        refCount_    {0}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_son
        (log_t const i) const -> vertex_t*
    {
        return forwardStar_[i].target;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_mark
        () const -> bool
    {
        return mark_;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::toggle_mark
        () -> void
    {
        mark_ ^= true;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::set_mark
        (bool const mark) -> void
    {
        mark_ = mark;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_index
        () const -> index_t
    {
        return index_;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::set_index
        (index_t const index) -> void
    {
        index_ = index;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_ref_count
        () const -> std::size_t
    {
        return refCount_;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::inc_ref_count
        () -> void
    {
        ++refCount_;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::dec_ref_count
        () -> void
    {
        --refCount_;
    }

// normal vertex definitions:

    template<class VertexData, class ArcData, std::size_t P>
    vertex<VertexData, ArcData, P>::vertex
        (index_t const index) :
        base_t {index}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex<VertexData, ArcData, P>::vertex
        ( index_t const  index
        , son_a   const& sons ) :
        base_t {index, sons}
    {
    }

// specialized empty vertex definitions:

    template<class ArcData, std::size_t P>
    vertex<void, ArcData, P>::vertex
        (index_t const index) :
        base_t {index}
    {
    }

    template<class ArcData, std::size_t P>
    vertex<void, ArcData, P>::vertex
        ( index_t const  index
        , son_a   const& sons ) :
        base_t {index, sons}
    {
    }
}

#endif