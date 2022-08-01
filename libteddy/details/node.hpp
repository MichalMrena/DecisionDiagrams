#ifndef MIX_DD_NODE_HPP
#define MIX_DD_NODE_HPP

#include <libteddy/details/types.hpp>
#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

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

    template<class, class T>
    struct second
    {
        using type = T;
    };

    template<class X, class T>
    using second_t = typename second<X, T>::type;

    namespace degrees
    {
        struct mixed {};

        template<uint_t N>
        struct fixed
        {
            static_assert(N > 1);
            auto constexpr operator()()
            {
                return N;
            }
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
    public:
        template<uint_t N>
        static auto container
            (uint_t, degrees::fixed<N>) -> std::array<node*, N>;

        static auto container
            (uint_t, degrees::mixed) -> std::unique_ptr<node*[]>;

    public:
        using sons_t = decltype(container(uint_t{}, D()));
        using bits_t = unsigned int;

    public:
        explicit node (uint_t);
        node  (index_t, sons_t&&);
        node  (node const&) = delete;
        node  (node&&) = delete;
        // ~node () = default; // TODO zatial nefunguje v clangu
        // ~node () requires(degrees::is_mixed<D>()());
        ~node ();

        // notice: Making it a dummy template and making the return type
        //         dependent on the template makes it SFINE.
        template<class Foo = void> requires (not std::is_void_v<Data>)
        auto data () -> second_t<Foo, Data>&;

        template<class Foo = void> requires (not std::is_void_v<Data>)
        auto data () const -> second_t<Foo, Data> const&;

        auto get_next      () const       -> node*;
        auto set_next      (node*)        -> void;
        auto is_internal   () const       -> bool;
        auto is_terminal   () const       -> bool;
        auto is_used       () const       -> bool;
        auto set_unused    ()             -> void;
        auto is_marked     () const       -> bool;
        auto toggle_marked ()             -> void;
        auto set_marked    ()             -> void;
        auto set_notmarked ()             -> void;
        auto get_ref_count () const       -> uint_t;
        auto inc_ref_count ()             -> void;
        auto dec_ref_count ()             -> void;
        auto get_index     () const       -> index_t;
        auto set_index     (index_t)      -> void;
        auto get_sons      () const       -> sons_t const&;
        auto get_son       (uint_t) const -> node*;
        auto set_sons      (sons_t)       -> void;
        auto get_value     () const       -> uint_t;

    private:
        // TODO: c++20, constructors not necessary

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

        struct terminal
        {
            uint_t value;
            terminal (uint_t const v) :
                value {v}
            {
            }
        };

    private:
        auto union_internal        ()       -> internal*;
        auto union_internal        () const -> internal const*;
        auto union_terminal        ()       -> terminal*;
        auto union_terminal        () const -> terminal const*;
        auto union_internal_unsafe ()       -> internal*;
        auto is_or_was_internal    () const -> bool;

    private:
        inline static constexpr auto MarkM   = 1u << (8 * sizeof(bits_t) - 1);
        inline static constexpr auto UsedM   = 1u << (8 * sizeof(bits_t) - 2);
        inline static constexpr auto LeafM   = 1u << (8 * sizeof(bits_t) - 3);
        inline static constexpr auto RefsM   = ~(MarkM | UsedM | LeafM);
        inline static constexpr auto RefsMax = ~RefsM;

    private:
        alignas(internal) std::byte union_[sizeof(internal)];
        [[no_unique_address]]
        opt_member<Data>            data_;
        node*                       next_;
        bits_t                      bits_;
    };

    template<class Data, degree D>
    template<uint_t N>
    auto node<Data, D>::container
        (uint_t, degrees::fixed<N>) -> std::array<node*, N>
    {
        return std::array<node*, N>();
    }

    template<class Data, degree D>
    auto node<Data, D>::container
        (uint_t const domain, degrees::mixed) -> std::unique_ptr<node*[]>
    {
        // Not supported yet.
        // return std::make_unique_for_overwrite<node*[]>(domain);
        return std::make_unique<node*[]>(domain);
    }

    template<class Data, degree D>
    node<Data, D>::node
        (uint_t const i) :
        next_ {nullptr},
        bits_ {LeafM | UsedM}
    {
        std::construct_at(this->union_terminal(), i);
    }

    template<class Data, degree D>
    node<Data, D>::node
        (index_t const i, sons_t&& sons) :
        next_ {nullptr},
        bits_ {UsedM}
    {
        std::construct_at(this->union_internal(), std::move(sons), i);
    }

    // template<class Data, degree D>
    // node<Data, D>::~node
    //     () requires(degrees::is_mixed<D>()())
    // {
    //     if (this->is_or_was_internal())
    //     {
    //         std::destroy_at(this->union_internal_unsafe());
    //     }
    // }

    template<class Data, degree D>
    node<Data, D>::~node
        ()
    {
        if (this->is_or_was_internal())
        {
            std::destroy_at(this->union_internal_unsafe());
        }
    }

    template<class Data, degree D>
    template<class Foo> requires (not std::is_void_v<Data>)
    auto node<Data, D>::data
        () -> second_t<Foo, Data>&
    {
        assert(this->is_used());
        return data_.m;
    }

    template<class Data, degree D>
    template<class Foo> requires (not std::is_void_v<Data>)
    auto node<Data, D>::data
        () const -> second_t<Foo, Data> const&
    {
        assert(this->is_used());
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
    auto node<Data, D>::get_sons
        () const -> sons_t const&
    {
        return this->union_internal()->sons;
    }

    template<class Data, degree D>
    auto node<Data, D>::get_son
        (uint_t const k) const -> node*
    {
        return (this->union_internal()->sons)[k];
    }

    template<class Data, degree D>
    auto node<Data, D>::set_sons
        (sons_t ss) -> void
    {
        using std::swap;
        swap(this->union_internal()->sons, ss);
    }

    template<class Data, degree D>
    auto node<Data, D>::get_value
        () const -> uint_t
    {
        return this->union_terminal()->value;
    }

    template<class Data, degree D>
    auto node<Data, D>::is_internal
        () const -> bool
    {
        return this->is_used() && not this->is_terminal();
    }

    template<class Data, degree D>
    auto node<Data, D>::is_terminal
        () const -> bool
    {
        return this->is_used() && (bits_ & LeafM);
    }

    template<class Data, degree D>
    auto node<Data, D>::is_used
        () const -> bool
    {
        return bits_ & UsedM;
    }

    template<class Data, degree D>
    auto node<Data, D>::set_unused
        () -> void
    {
        bits_ &= ~UsedM;
    }

    template<class Data, degree D>
    auto node<Data, D>::is_marked
        () const -> bool
    {
        return bits_ & MarkM;
    }

    template<class Data, degree D>
    auto node<Data, D>::toggle_marked
        () -> void
    {
        bits_ ^= MarkM;
    }

    template<class Data, degree D>
    auto node<Data, D>::set_marked
        () -> void
    {
        bits_ |= MarkM;
    }

    template<class Data, degree D>
    auto node<Data, D>::set_notmarked
        () -> void
    {
        bits_ &= ~MarkM;
    }

    template<class Data, degree D>
    auto node<Data, D>::get_ref_count
        () const -> uint_t
    {
        return static_cast<uint_t>(bits_ & RefsM);
    }

    template<class Data, degree D>
    auto node<Data, D>::inc_ref_count
        () -> void
    {
        assert(this->get_ref_count() < RefsMax);
        ++bits_;
    }

    template<class Data, degree D>
    auto node<Data, D>::dec_ref_count
        () -> void
    {
        assert(this->get_ref_count() > 0);
        --bits_;
    }

    template<class Data, degree D>
    auto node<Data, D>::get_index
        () const -> index_t
    {
        return this->union_internal()->index;
    }

    template<class Data, degree D>
    auto node<Data, D>::set_index
        (index_t const i) -> void
    {
        this->union_internal()->index = i;
    }

    template<class Data, degree D>
    auto node<Data, D>::union_internal
        () -> internal*
    {
        assert(this->is_internal());
        return reinterpret_cast<internal*>(&union_);
    }

    template<class Data, degree D>
    auto node<Data, D>::union_internal
        () const -> internal const*
    {
        assert(this->is_internal());
        return reinterpret_cast<internal const*>(&union_);
    }

    template<class Data, degree D>
    auto node<Data, D>::union_terminal
        () -> terminal*
    {
        assert(this->is_terminal());
        return reinterpret_cast<terminal*>(&union_);
    }

    template<class Data, degree D>
    auto node<Data, D>::union_terminal
        () const -> terminal const*
    {
        assert(this->is_terminal());
        return reinterpret_cast<terminal const*>(&union_);
    }

    template<class Data, degree D>
    auto node<Data, D>::union_internal_unsafe
        () -> internal*
    {
        return reinterpret_cast<internal*>(&union_);
    }

    template<class Data, degree D>
    auto node<Data, D>::is_or_was_internal
        () const -> bool
    {
        return not (bits_ & LeafM);
    }
}

#endif