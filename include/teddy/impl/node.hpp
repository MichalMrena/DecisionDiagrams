#ifndef MIX_DD_NODE_HPP
#define MIX_DD_NODE_HPP

#include "types.hpp"

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>

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

    namespace degrees
    {
        struct mixed {};
        template<uint_t N> struct fixed
        {
            static_assert(N > 1);
        };

        template<class T>
        struct is_fixed : public std::false_type {};

        template<uint_t N>
        struct is_fixed<fixed<N>> : public std::true_type {};

        template<class T>
        using is_mixed = std::is_same<T, mixed>;
    }

    template<class T>
    concept degree = degrees::is_mixed<T>()() or degrees::is_fixed<T>()();

    template<class Data, degree D>
    class node
    {
    private:
        template<uint_t N>
        static auto container (degrees::fixed<N>) -> std::array<node*, N>;
        static auto container (degrees::mixed)    -> std::unique_ptr<node*[]>;

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
        struct internal
        {
            sons_t  sons;
            index_t index;
            internal (sons_t ss, index_t i) :
                sons  {std::move(ss)},
                index {i}
            {
            }
        };
        using terminal = uint_t;
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
        std::construct_at(&this->union_internal(), std::move(sons), i);
    }

    template<class Data, degree D>
    node<Data, D>::~node
        () requires(degrees::is_mixed<D>()())
    {
        if (this->is_internal())
        {
            std::destroy_at(this->as_internal());
            // this->as_internal().~dyn_sons();
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
}

#endif
