#ifndef MIX_DD_GRAPH_HPP
#define MIX_DD_GRAPH_HPP

#include "typedefs.hpp"
#include "../utils/more_algorithm.hpp"
#include "../utils/more_type_traits.hpp"

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
        ArcData data;
    };

    /**
        @brief Arc with without data member variable.
     */
    template<class VertexData, std::size_t P>
    struct arc<VertexData, void, P> : public arc_base<VertexData, void, P>
    {
    };

    /**
       @brief Vertex base.
     */
    template<class VertexData, class ArcData, std::size_t P>
    class vertex_base
    {
    public:
        using vertex_t    = vertex<VertexData, ArcData, P>;
        using arc_t       = arc<VertexData, ArcData, P>;
        using arc_a       = std::array<arc_t, P>;
        using son_a       = std::array<vertex_t*, P>;
        using log_t       = typename log_val_traits<P>::type;
        using ref_count_t = std::uint32_t;

    public:
        vertex_base (index_t const index);
        vertex_base (index_t const index, son_a const& sons);

    public:
        auto get_id        () const                    -> std::intptr_t;
        auto get_son       (std::size_t const i) const -> vertex_t*;
        auto set_sons      (son_a const& sons)         -> void;
        auto get_mark      () const                    -> bool;
        auto toggle_mark   ()                          -> void;
        auto get_index     () const                    -> index_t;
        auto set_index     (index_t const index)       -> void;
        auto get_ref_count () const                    -> ref_count_t;
        auto inc_ref_count ()                          -> void;
        auto dec_ref_count ()                          -> void;

        template<class VertexOp>
        auto for_each_son (VertexOp op) -> void;

        template<class IndexedVertexOp>
        auto for_each_son_i (IndexedVertexOp op) -> void;

    private:
        inline static constexpr auto MaskMark = ref_count_t(1ul << (8 * sizeof(ref_count_t) - 1));
        inline static constexpr auto MaskRef  = ~MaskMark;

    private:
        arc_a       forwardStar_;
        ref_count_t markRefCount_;
        index_t     index_;
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
        vertex(index_t const index);
        vertex(index_t const index, son_a const& sons);
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
        vertex(index_t const index);
        vertex(index_t const index, son_a const& sons);
    };

// vertex_base definitions:

    template<class VertexData, class ArcData, std::size_t P>
    vertex_base<VertexData, ArcData, P>::vertex_base
        (index_t const index) :
        vertex_base {index, {}}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex_base<VertexData, ArcData, P>::vertex_base
        (index_t const index, son_a const& sons) :
        forwardStar_  {utils::map_to_array(sons, [](auto const v) { return arc_t {{v}}; })},
        markRefCount_ {0},
        index_        {index}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_id
        () const -> std::intptr_t
    {
        return reinterpret_cast<std::uintptr_t>(this);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_son
        (std::size_t const i) const -> vertex_t*
    {
        return forwardStar_[i].target;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::set_sons
        (son_a const& sons) -> void
    {
        forwardStar_ = utils::map_to_array(sons, [](auto const v)
        {
            return arc_t {{v}};
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_mark
        () const -> bool
    {
        return markRefCount_ & MaskMark;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::toggle_mark
        () -> void
    {
        markRefCount_ ^= MaskMark;
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
        () const -> ref_count_t
    {
        return markRefCount_ & MaskRef;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::inc_ref_count
        () -> void
    {
        ++markRefCount_;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::dec_ref_count
        () -> void
    {
        --markRefCount_;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto vertex_base<VertexData, ArcData, P>::for_each_son
        (VertexOp op) -> void
    {
        if (this->get_son(0))
        {
            for (auto i = 0u; i < P; ++i)
            {
                op(this->get_son(i));
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class IndexedVertexOp>
    auto vertex_base<VertexData, ArcData, P>::for_each_son_i
        (IndexedVertexOp op) -> void
    {
        if (this->get_son(0))
        {
            for (auto i = 0u; i < P; ++i)
            {
                op(i, this->get_son(i));
            }
        }
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
        (index_t const index, son_a const& sons) :
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
        (index_t const index, son_a const& sons) :
        base_t {index, sons}
    {
    }
}

#endif