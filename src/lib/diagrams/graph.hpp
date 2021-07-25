#ifndef MIX_DD_GRAPH_HPP
#define MIX_DD_GRAPH_HPP

#include "typedefs.hpp"
#include "../utils/more_algorithm.hpp"

#include <array>
#include <vector>
#include <concepts>
#include <type_traits>
#include <cstddef>
#include <memory>
#include <cassert>

namespace teddy
{
    template<class T>
    struct opt_member
    {
        T m;
    };

    template<>
    struct opt_member<void>
    {
    };

    struct binary {};
    struct mixed {};
    template<std::size_t N> struct nary {};

    template<class T>
    struct is_nary : public std::false_type {};

    template<std::size_t N>
    struct is_nary<nary<N>> : public std::true_type {};

    template<class T>
    concept degree = std::same_as<T, binary>
                  || std::same_as<T, mixed>
                  || is_nary<T>()();

    template<class Data, degree D>
    class dd_node
    {
    public:
        template<std::size_t N>
        static auto container (nary<N>) -> std::array<dd_node*, N>;
        static auto container (binary)  -> std::array<dd_node*, 2>;
        static auto container (mixed)   -> dd_node*;

    public:
        using sons_t = decltype(container(D()));

    public:
        explicit dd_node (int_t);
        dd_node (index_t, sons_t);
        ~dd_node () = default;
        ~dd_node () requires(std::is_same_v<D, mixed>);

        auto data ()       -> Data&       requires(!std::is_void_v<Data>);
        auto data () const -> Data const& requires(!std::is_void_v<Data>);

        auto get_next () const   -> dd_node*;
        auto set_next (dd_node*) -> void;

        auto is_internal () const -> bool;
        auto is_terminal () const -> bool;

    private:
        using refs_t   = unsigned int;
        using terminal = int_t;
        using internal = struct
        {
            sons_t sons;
            index_t index;
        };
        using union_t  = std::array<std::byte, sizeof(internal)>;

    private:
        auto as_internal ()       -> internal&;
        auto as_internal () const -> internal const&;
        auto as_terminal ()       -> terminal&;
        auto as_terminal () const -> terminal const&;

    private:
        inline static constexpr auto MarkMask = refs_t(1) << (8 * sizeof(refs_t) - 1);
        inline static constexpr auto UsedMask = refs_t(1) << (8 * sizeof(refs_t) - 2);
        inline static constexpr auto LeafMask = refs_t(1) << (8 * sizeof(refs_t) - 3);
        inline static constexpr auto RefsMax  = 1ul << (8 * sizeof(refs_t) - 3);

    private:
        union_t          union_;
        [[no_unique_address]]
        opt_member<Data> data_;
        dd_node*         next_;
        refs_t           refs_;
    };

    template<class Data, degree D>
    dd_node<Data, D>::dd_node
        (int_t const i) :
        next_ {nullptr},
        refs_ {LeafMask}
    {
        std::construct_at(reinterpret_cast<terminal*>(union_.data()), i);
    }

    template<class Data, degree D>
    dd_node<Data, D>::dd_node
        (index_t const i, sons_t sons) :
        next_ {nullptr},
        refs_ {0}
    {
        std::construct_at(reinterpret_cast<internal*>(union_.data()), i, sons);
    }

    template<class Data, degree D>
    dd_node<Data, D>::~dd_node
        () requires(std::is_same_v<D, mixed>)
    {
        if (this->is_internal())
        {
            delete[] this->as_internal().sons;
        }
    }

    template<class Data, degree D>
    auto dd_node<Data, D>::data
        () -> Data& requires(!std::is_void_v<Data>)
    {
        return data_.m;
    }

    template<class Data, degree D>
    auto dd_node<Data, D>::data
        () const -> Data const& requires(!std::is_void_v<Data>)
    {
        return data_.m;
    }

    template<class Data, degree D>
    auto dd_node<Data, D>::get_next
        () const -> dd_node*
    {
        return next_;
    }

    template<class Data, degree D>
    auto dd_node<Data, D>::set_next
        (dd_node* const n) -> void
    {
        next_ = n;
    }

    template<class Data, degree D>
    auto dd_node<Data, D>::as_internal
        () -> internal&
    {
        assert(this->is_internal());
        return *reinterpret_cast<internal*>(union_.data());
    }

    template<class Data, degree D>
    auto dd_node<Data, D>::as_internal
        () const -> internal const&
    {
        assert(this->is_internal());
        return *reinterpret_cast<internal*>(union_.data());
    }

    template<class Data, degree D>
    auto dd_node<Data, D>::as_terminal
        () -> terminal&
    {
        assert(this->is_terminal());
        return *reinterpret_cast<terminal*>(union_.data());
    }

    template<class Data, degree D>
    auto dd_node<Data, D>::as_terminal
        () const -> terminal const&
    {
        assert(this->is_terminal());
        return *reinterpret_cast<terminal*>(union_.data());
    }






    /** vertex forward declaration */
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
        using vertex_a    = std::array<vertex_t*, P>;
        using log_t       = typename log_val_traits<P>::type;
        using ref_count_t = unsigned int;

    protected:
        vertex_base (index_t i);
        vertex_base (index_t i, vertex_a const& sons);

    public:
        auto get_id        () const               -> std::uintptr_t;
        auto get_son       (std::size_t i) const  -> vertex_t*;
        auto set_sons      (vertex_a const& sons) -> void;
        auto get_next      () const               -> vertex_t*;
        auto set_next      (vertex_t* n)          -> void;
        auto get_mark      () const               -> bool;
        auto toggle_mark   ()                     -> void;
        auto get_index     () const               -> index_t;
        auto set_index     (index_t i)            -> void;
        auto get_ref_count () const               -> ref_count_t;
        auto inc_ref_count ()                     -> void;
        auto dec_ref_count ()                     -> void;

        template<class VertexOp>
        auto for_each_son (VertexOp op) -> void;

        template<class IndexedVertexOp>
        auto for_each_son_i (IndexedVertexOp op) -> void;

    private:
        inline static constexpr auto MaskMark = ref_count_t(1ul << (8 * sizeof(ref_count_t) - 1));
        inline static constexpr auto MaskRef  = ~MaskMark;

    private:
        arc_a       forwardStar_;
        vertex_t*   next_;
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
        using vertex_a  = typename vertex_base<VertexData, ArcData, P>::vertex_a;
        using log_t  = typename log_val_traits<P>::type;
        using base_t = vertex_base<VertexData, ArcData, P>;

    public:
        VertexData data;

    public:
        vertex ();
        vertex (index_t i);
        vertex (index_t i, vertex_a const& sons);
    };

    /**
        @brief Specialized vertex without data member variable.
     */
    template<class ArcData, std::size_t P>
    class vertex<void, ArcData, P> : public vertex_base<void, ArcData, P>
    {
    public:
        using vertex_a  = typename vertex_base<void, ArcData, P>::vertex_a;
        using base_t = vertex_base<void, ArcData, P>;

    public:
        vertex ();
        vertex (index_t const i);
        vertex (index_t const i, vertex_a const& sons);
    };

// vertex_base definitions:

    template<class VertexData, class ArcData, std::size_t P>
    vertex_base<VertexData, ArcData, P>::vertex_base
        (index_t const i) :
        vertex_base {i, {}}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex_base<VertexData, ArcData, P>::vertex_base
        (index_t const i, vertex_a const& sons) :
        forwardStar_  {utils::fmap_to_array(sons, [](auto const v) { return arc_t {{v}}; })},
        next_         {nullptr},
        markRefCount_ {0},
        index_        {i}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_id
        () const -> std::uintptr_t
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
        (vertex_a const& sons) -> void
    {
        forwardStar_ = utils::fmap_to_array(sons, [](auto const v)
        {
            return arc_t {{v}};
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::get_next
        () const -> vertex_t*
    {
        return next_;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_base<VertexData, ArcData, P>::set_next
        (vertex_t* const n) -> void
    {
        next_ = n;
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
        (index_t const i) -> void
    {
        index_ = i;
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
        auto const end = std::end(forwardStar_);
        auto it = std::begin(forwardStar_);

        while (it != end && it->target)
        {
            op(it->target);
            ++it;
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class IndexedVertexOp>
    auto vertex_base<VertexData, ArcData, P>::for_each_son_i
        (IndexedVertexOp op) -> void
    {
        auto const end = std::end(forwardStar_);
        auto it = std::begin(forwardStar_);
        auto i  = 0u;

        while (it != end && it->target)
        {
            op(i, it->target);
            ++it;
            ++i;
        }
    }

// normal vertex definitions:

    template<class VertexData, class ArcData, std::size_t P>
    vertex<VertexData, ArcData, P>::vertex
        () :
        base_t {static_cast<index_t>(-1)}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex<VertexData, ArcData, P>::vertex
        (index_t const i) :
        base_t {i}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex<VertexData, ArcData, P>::vertex
        (index_t const i, vertex_a const& sons) :
        base_t {i, sons}
    {
    }

// specialized empty vertex definitions:

    template<class ArcData, std::size_t P>
    vertex<void, ArcData, P>::vertex
        () :
        base_t {static_cast<index_t>(-1)}
    {
    }

    template<class ArcData, std::size_t P>
    vertex<void, ArcData, P>::vertex
        (index_t const i) :
        base_t {i}
    {
    }

    template<class ArcData, std::size_t P>
    vertex<void, ArcData, P>::vertex
        (index_t const i, vertex_a const& sons) :
        base_t {i, sons}
    {
    }
}

#endif