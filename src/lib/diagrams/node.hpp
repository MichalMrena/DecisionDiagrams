#ifndef MIX_DD_GRAPH_HPP
#define MIX_DD_GRAPH_HPP

#include "types.hpp"

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>

    #include "../utils/more_algorithm.hpp"


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

    template<class Node>
    using dyn_sons = std::unique_ptr<Node*[]>;

    namespace degrees
    {
        struct mixed {};
        template<uint_t N> struct fixed {};

        template<class T>
        struct is_fixed : public std::false_type {};

        template<uint_t N>
        struct is_fixed<fixed<N>> : public std::true_type {};

        template<class T>
        using is_mixed = std::is_same<T, mixed>;
    }

    template<class T>
    concept degree = degrees::is_mixed<T>()() || degrees::is_fixed<T>()();

    template<class Data, degree D>
    class node
    {
    public:
        template<uint_t N>
        static auto container (degrees::fixed<N>) -> std::array<node*, N>;
        static auto container (degrees::mixed)    -> dyn_sons<node*>;

    public:
        using sons_t = decltype(container(D()));
        using refs_t = unsigned int;

    public:
        explicit node (uint_t);
        node  (index_t, sons_t);
        node  (node const&) = delete;
        node  (node&&) = delete;
        ~node () = default;
        ~node () requires(degrees::is_mixed<D>()());

        auto data ()       -> Data&       requires(!std::is_void<Data>()());
        auto data () const -> Data const& requires(!std::is_void<Data>()());

        auto get_next      () const       -> node*;
        auto set_next      (node*)        -> void;
        auto is_internal   () const       -> bool;
        auto is_terminal   () const       -> bool;
        auto is_used       () const       -> bool;
        auto set_used      ()             -> void;
        auto set_unused    ()             -> void;
        auto is_marked     () const       -> bool;
        auto toggle_marked ()             -> void;
        auto get_ref_count () const       -> refs_t;
        auto inc_ref_count ()             -> void;
        auto dec_ref_count ()             -> void;
        auto get_index     () const       -> index_t;
        auto set_index     (index_t)      -> void;
        auto get_son       (uint_t) const -> node*;
        auto set_sons      (sons_t)       -> void;
        auto get_value     () const       -> uint_t;

    private:
        using terminal = uint_t;
        using internal = struct
        {
            sons_t  sons;
            index_t index;
        };
        using union_t  = std::array<std::byte, sizeof(internal)>;

    private:
        auto union_internal ()       -> internal&;
        auto union_internal () const -> internal const&;
        auto union_terminal ()       -> terminal&;
        auto union_terminal () const -> terminal const&;

    private:
        inline static constexpr auto MarkM   = 1u << (8 * sizeof(refs_t) - 1);
        inline static constexpr auto UsedM   = 1u << (8 * sizeof(refs_t) - 2);
        inline static constexpr auto LeafM   = 1u << (8 * sizeof(refs_t) - 3);
        inline static constexpr auto RefsM   = ~(MarkM | UsedM | LeafM);
        inline static constexpr auto RefsMax = ~RefsM;

    private:
        union_t          union_;
        [[no_unique_address]]
        opt_member<Data> data_;
        node*            next_;
        refs_t           refs_;
    };

    template<class Data, degree D>
    node<Data, D>::node
        (uint_t const i) :
        next_ {nullptr},
        refs_ {LeafM | UsedM}
    {
        std::construct_at(&this->union_terminal(), i);
    }

    template<class Data, degree D>
    node<Data, D>::node
        (index_t const i, sons_t sons) :
        next_ {nullptr},
        refs_ {UsedM}
    {
        std::construct_at(&this->union_internal(), i, sons);
    }

    template<class Data, degree D>
    node<Data, D>::~node
        () requires(degrees::is_mixed<D>()())
    {
        if (this->is_internal())
        {
            this->as_internal().~dyn_sons();
        }
    }

    template<class Data, degree D>
    auto node<Data, D>::data
        () -> Data& requires(!std::is_void<Data>()())
    {
        return data_.m;
    }

    template<class Data, degree D>
    auto node<Data, D>::data
        () const -> Data const& requires(!std::is_void<Data>()())
    {
        return data_.m;
    }

    template<class Data, degree D>
    auto node<Data, D>::get_next
        () const -> node*
    {
        return next_;
    }

    template<class Data, degree D>
    auto node<Data, D>::set_next
        (node* const n) -> void
    {
        next_ = n;
    }

    template<class Data, degree D>
    auto node<Data, D>::get_son
        (uint_t const k) const -> node*
    {
        return this->union_internal().sons[k];
    }

    template<class Data, degree D>
    auto node<Data, D>::set_sons
        (sons_t ss) -> void
    {
        using std::swap;
        swap(this->union_internal().sons, ss);
    }

    template<class Data, degree D>
    auto node<Data, D>::get_value
        () const -> uint_t
    {
        return this->union_terminal();
    }

    template<class Data, degree D>
    auto node<Data, D>::is_internal
        () const -> bool
    {
        return !this->is_terminal();
    }

    template<class Data, degree D>
    auto node<Data, D>::is_terminal
        () const -> bool
    {
        return refs_ & LeafM;
    }

    template<class Data, degree D>
    auto node<Data, D>::is_used
        () const -> bool
    {
        return refs_ & UsedM;
    }

    template<class Data, degree D>
    auto node<Data, D>::set_used
        () -> void
    {
        refs_ |= UsedM;
    }

    template<class Data, degree D>
    auto node<Data, D>::set_unused
        () -> void
    {
        refs_ &= ~UsedM;
    }

    template<class Data, degree D>
    auto node<Data, D>::is_marked
        () const -> bool
    {
        return refs_ & MarkM;
    }

    template<class Data, degree D>
    auto node<Data, D>::toggle_marked
        () -> void
    {
        refs_ ^= MarkM;
    }

    template<class Data, degree D>
    auto node<Data, D>::get_ref_count
        () const -> refs_t
    {
        return refs_ & RefsM;
    }

    template<class Data, degree D>
    auto node<Data, D>::inc_ref_count
        () -> void
    {
        assert(this->get_ref_count() < RefsMax);
        ++refs_;
    }

    template<class Data, degree D>
    auto node<Data, D>::dec_ref_count
        () -> void
    {
        assert(this->get_ref_count() > 0);
        --refs_;
    }

    template<class Data, degree D>
    auto node<Data, D>::get_index
        () const -> index_t
    {
        return this->union_internal().index;
    }

    template<class Data, degree D>
    auto node<Data, D>::set_index
        (index_t const i) -> void
    {
        this->union_internal().index = i;
    }

    template<class Data, degree D>
    auto node<Data, D>::union_internal
        () -> internal&
    {
        assert(this->is_internal());
        return *reinterpret_cast<internal*>(union_.data());
    }

    template<class Data, degree D>
    auto node<Data, D>::union_internal
        () const -> internal const&
    {
        assert(this->is_internal());
        return *reinterpret_cast<internal*>(union_.data());
    }

    template<class Data, degree D>
    auto node<Data, D>::union_terminal
        () -> terminal&
    {
        assert(this->is_terminal());
        return *reinterpret_cast<terminal*>(union_.data());
    }

    template<class Data, degree D>
    auto node<Data, D>::union_terminal
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
