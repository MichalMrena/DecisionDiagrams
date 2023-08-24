#ifndef LIBTEDDY_DETAILS_NODE_HPP
#define LIBTEDDY_DETAILS_NODE_HPP

#include <libteddy/details/tools.hpp>
#include <libteddy/details/types.hpp>

#include <cassert>
#include <new>

namespace teddy
{
namespace degrees
{
struct mixed
{
    // Dummy value to make life easier
    static constexpr int32 value = 1;
};

template<int32 N>
struct fixed
{
    static_assert(N > 1);
    static constexpr int32 value = N;
};

template<class T>
struct is_fixed
{
    static constexpr bool value = false;
};

template<int32 N>
struct is_fixed<fixed<N>>
{
    static constexpr bool value = true;
};

template<class T>
struct is_mixed
{
    static constexpr bool value = false;
};

template<>
struct is_mixed<mixed>
{
    static constexpr bool value = true;
};
} // namespace degrees

template<class Data, class Degree>
class node;

template<class Data, class Degree>
struct node_ptr_array
{
    node<Data, Degree>* sons_[Degree::value];

    auto operator[](int64 const index) -> node<Data, Degree>*&
    {
        return sons_[index];
    }

    auto operator[](int64 const index) const -> node<Data, Degree>* const&
    {
        return sons_[index];
    }
};

template<class Data, class Degree>
class node
{
public:
    template<int32 N>
    static auto make_son_container (
        int32,
        degrees::fixed<N>
    ) -> node_ptr_array<Data, Degree>
    {
        return node_ptr_array<Data, Degree>();
    }

    static auto make_son_container (
        int32 const domain,
        degrees::mixed
    ) -> node**
    {
        return static_cast<node**>(
            ::operator new (as_usize(domain) * sizeof(node*))
        );
    }

    static auto delete_son_container (node** sons) -> void
    {
        ::operator delete (sons);
    }

public:
    using son_container = decltype(make_son_container(int32(), Degree()));

public:
    explicit node(int32 value);
    node(int32 index, son_container sons);
    ~node() = default;
    ~node() requires(degrees::is_mixed<Degree>::value);

    node()                       = delete;
    node(node const&)            = delete;
    node(node&&)                 = delete;
    auto operator= (node const&) = delete;
    auto operator= (node&&)      = delete;

    template<class Foo = void>
    requires(not utils::is_void<Data>::value)
    auto get_data () -> utils::second_t<Foo, Data>&;

    template<class Foo = void>
    requires(not utils::is_void<Data>::value)
    auto get_data () const -> utils::second_t<Foo, Data> const&;

    [[nodiscard]] auto is_internal () const -> bool;
    [[nodiscard]] auto is_terminal () const -> bool;
    [[nodiscard]] auto is_used () const -> bool;
    [[nodiscard]] auto is_marked () const -> bool;
    [[nodiscard]] auto get_next () const -> node*;
    [[nodiscard]] auto get_ref_count () const -> int32;
    [[nodiscard]] auto get_index () const -> int32;
    [[nodiscard]] auto get_sons () const -> son_container const&;
    [[nodiscard]] auto get_son (int32 sonOrder) const -> node*;
    [[nodiscard]] auto get_value () const -> int32;
    auto set_next (node* next) -> void;
    auto set_unused () -> void;
    auto set_marked () -> void;
    auto set_notmarked () -> void;
    auto set_index (int32 index) -> void;
    auto set_sons (son_container const& sons) -> void;
    auto toggle_marked () -> void;
    auto inc_ref_count () -> void;
    auto dec_ref_count () -> void;

private:
    // P0960R3 constructors not necessary
    // >=g++10, >=clang++16 ...
    //                   ^^ :(

    struct internal
    {
        son_container sons_;
        int32 index_;

        internal(son_container sons, int32 index) :
            sons_(sons),
            index_(index)
        {
        }
    };

    struct terminal
    {
        int32 value_;

        terminal(int32 const value) :
            value_(value)
        {
        }
    };

private:
    [[nodiscard]] auto as_internal () -> internal*;
    [[nodiscard]] auto as_internal () const -> internal const*;
    [[nodiscard]] auto as_terminal () -> terminal*;
    [[nodiscard]] auto as_terminal () const -> terminal const*;
    [[nodiscard]] auto as_internal_unsafe () -> internal*;
    [[nodiscard]] auto is_or_was_internal () const -> bool;

private:
    static constexpr uint32 MarkM   = 1U << (8 * sizeof(uint32) - 1);
    static constexpr uint32 UsedM   = 1U << (8 * sizeof(uint32) - 2);
    static constexpr uint32 LeafM   = 1U << (8 * sizeof(uint32) - 3);
    static constexpr uint32 RefsM   = ~(MarkM | UsedM | LeafM);
    static constexpr uint32 RefsMax = RefsM + 1;

private:
    // TODO: use union

    alignas(internal) char union_[sizeof(internal)];
    [[no_unique_address]] utils::optional_member<Data> data_;
    node* next_;
    uint32 bits_;
};

template<class Data, class Degree>
node<Data, Degree>::node(int32 const value) :
    union_ {},
    next_ {nullptr},
    bits_ {LeafM | UsedM}
{
    ::new (this->as_terminal()) terminal(value);
}

template<class Data, class Degree>
node<Data, Degree>::node(int32 const index, son_container sons) :
    union_ {},
    next_ {nullptr},
    bits_ {UsedM}
{
    ::new (this->as_internal()) internal(sons, index);
}

template<class Data, class Degree>
node<Data, Degree>::~node()
requires(degrees::is_mixed<Degree>::value)
{
    if constexpr (degrees::is_mixed<Degree>::value)
    {
        if (this->is_or_was_internal())
        {
            ::operator delete (this->as_internal_unsafe()->sons_);
        }
    }
}

template<class Data, class Degree>
template<class Foo>
requires(not utils::is_void<Data>::value)
auto node<Data, Degree>::get_data() -> utils::second_t<Foo, Data>&
{
    assert(this->is_used());
    return data_.member_;
}

template<class Data, class Degree>
template<class Foo>
requires(not utils::is_void<Data>::value)
auto node<Data, Degree>::get_data() const -> utils::second_t<Foo, Data> const&
{
    assert(this->is_used());
    return data_.member_;
}

template<class Data, class Degree>
auto node<Data, Degree>::get_next() const -> node*
{
    return next_;
}

template<class Data, class Degree>
auto node<Data, Degree>::set_next(node* const next) -> void
{
    next_ = next;
}

template<class Data, class Degree>
auto node<Data, Degree>::get_sons() const -> son_container const&
{
    return this->as_internal()->sons_;
}

template<class Data, class Degree>
auto node<Data, Degree>::get_son(int32 const sonOrder) const -> node*
{
    return (this->as_internal()->sons_)[as_uindex(sonOrder)];
}

template<class Data, class Degree>
auto node<Data, Degree>::set_sons(son_container const& sons) -> void
{
    if constexpr (degrees::is_mixed<Degree>::value)
    {
        ::operator delete (this->as_internal()->sons_);
    }

    this->as_internal()->sons_ = sons;
}

template<class Data, class Degree>
auto node<Data, Degree>::get_value() const -> int32
{
    return this->as_terminal()->value_;
}

template<class Data, class Degree>
auto node<Data, Degree>::is_internal() const -> bool
{
    return this->is_used() && not this->is_terminal();
}

template<class Data, class Degree>
auto node<Data, Degree>::is_terminal() const -> bool
{
    return this->is_used() && (bits_ & LeafM);
}

template<class Data, class Degree>
auto node<Data, Degree>::is_used() const -> bool
{
    return static_cast<bool>(bits_ & UsedM);
}

template<class Data, class Degree>
auto node<Data, Degree>::set_unused() -> void
{
    bits_ &= ~UsedM;
}

template<class Data, class Degree>
auto node<Data, Degree>::is_marked() const -> bool
{
    return static_cast<bool>(bits_ & MarkM);
}

template<class Data, class Degree>
auto node<Data, Degree>::toggle_marked() -> void
{
    bits_ ^= MarkM;
}

template<class Data, class Degree>
auto node<Data, Degree>::set_marked() -> void
{
    bits_ |= MarkM;
}

template<class Data, class Degree>
auto node<Data, Degree>::set_notmarked() -> void
{
    bits_ &= ~MarkM;
}

template<class Data, class Degree>
auto node<Data, Degree>::get_ref_count() const -> int32
{
    return static_cast<int32>(bits_ & RefsM);
}

template<class Data, class Degree>
auto node<Data, Degree>::inc_ref_count() -> void
{
    assert(this->get_ref_count() < static_cast<int32>(RefsMax));
    ++bits_;
}

template<class Data, class Degree>
auto node<Data, Degree>::dec_ref_count() -> void
{
    assert(this->get_ref_count() > 0);
    --bits_;
}

template<class Data, class Degree>
auto node<Data, Degree>::get_index() const -> int32
{
    return this->as_internal()->index_;
}

template<class Data, class Degree>
auto node<Data, Degree>::set_index(int32 const index) -> void
{
    this->as_internal()->index_ = index;
}

template<class Data, class Degree>
auto node<Data, Degree>::as_internal() -> internal*
{
    assert(this->is_internal());
    return reinterpret_cast<internal*>(&union_);
}

template<class Data, class Degree>
auto node<Data, Degree>::as_internal() const -> internal const*
{
    assert(this->is_internal());
    return reinterpret_cast<internal const*>(&union_);
}

template<class Data, class Degree>
auto node<Data, Degree>::as_terminal() -> terminal*
{
    assert(this->is_terminal());
    return reinterpret_cast<terminal*>(&union_);
}

template<class Data, class Degree>
auto node<Data, Degree>::as_terminal() const -> terminal const*
{
    assert(this->is_terminal());
    return reinterpret_cast<terminal const*>(&union_);
}

template<class Data, class Degree>
auto node<Data, Degree>::as_internal_unsafe() -> internal*
{
    return reinterpret_cast<internal*>(&union_);
}

template<class Data, class Degree>
auto node<Data, Degree>::is_or_was_internal() const -> bool
{
    return not static_cast<bool>(bits_ & LeafM);
}
} // namespace teddy

#endif