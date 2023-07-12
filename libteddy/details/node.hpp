#ifndef LIBTEDDY_DETAILS_NODE_HPP
#define LIBTEDDY_DETAILS_NODE_HPP

#include <libteddy/details/types.hpp>
#include <libteddy/details/tools.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>
#include <type_traits>

namespace teddy
{
namespace degrees
{
struct mixed
{
};

template<int32 N>
struct fixed
{
    static_assert(N > 1);

    auto constexpr operator() ()
    {
        return N;
    }
};

template<class T>
struct is_fixed : public std::false_type
{
};

template<int32 N>
struct is_fixed<fixed<N>> : public std::true_type
{
};

template<class T>
using is_mixed = std::is_same<T, mixed>;
} // namespace degrees

template<class T>
concept degree = degrees::is_mixed<T>()() || degrees::is_fixed<T>()();

template<class Data, degree Degree>
class node
{
public:
    template<int32 N>
    static auto make_son_container(int32 domain, degrees::fixed<N> degreeTag)
        -> std::array<node*, as_usize(N)>;

    static auto make_son_container(int32 domain, degrees::mixed degreeTag)
        -> std::unique_ptr<node*[]>;

public:
    using son_container = decltype(make_son_container(int32(), Degree()));

public:
    explicit node(int32 value);
    node(int32 index, son_container&& sons);
    // ~node () = default; // TODO zatial nefunguje v clangu
    // ~node () requires(degrees::is_mixed<D>()());
    ~node();

    node(node const&) = delete;
    node(node&&)      = delete;
    auto operator= (node const&) = delete;
    auto operator= (node&&) = delete;

    // notice: Making it a dummy template and making the return type
    //         dependent on the template makes it SFINE.
    template<class Foo = void>
    requires(not std::is_void_v<Data>)
    auto data () -> second_t<Foo, Data>&;

    template<class Foo = void>
    requires(not std::is_void_v<Data>)
    auto data () const -> second_t<Foo, Data> const&;

    [[nodiscard]] auto get_next () const -> node*;
    auto set_next (node* next) -> void;
    [[nodiscard]] auto is_internal () const -> bool;
    [[nodiscard]] auto is_terminal () const -> bool;
    [[nodiscard]] auto is_used () const -> bool;
    auto set_unused () -> void;
    [[nodiscard]] auto is_marked () const -> bool;
    auto toggle_marked () -> void;
    auto set_marked () -> void;
    auto set_notmarked () -> void;
    [[nodiscard]] auto get_ref_count () const -> int32;
    auto inc_ref_count () -> void;
    auto dec_ref_count () -> void;
    [[nodiscard]] auto get_index () const -> int32;
    auto set_index(int32 index) -> void;
    [[nodiscard]] auto get_sons () const -> son_container const&;
    [[nodiscard]] auto get_son(int32 sonOrder) const -> node*;
    auto set_sons(son_container sons) -> void;
    [[nodiscard]] auto get_value () const -> int32;

private:
    // TODO: c++20, constructors not necessary

    struct internal
    {
        son_container sons_;
        int32 index_;

        internal(son_container sons, int32 index) :
            sons_ (std::move(sons)),
            index_ (index)
        {
        }
    };

    struct terminal
    {
        int32 value_;

        terminal(int32 const value) : value_ {value}
        {
        }
    };

private:
    [[nodiscard]] auto union_internal () -> internal*;
    [[nodiscard]] auto union_internal () const -> internal const*;
    [[nodiscard]] auto union_terminal () -> terminal*;
    [[nodiscard]] auto union_terminal () const -> terminal const*;
    [[nodiscard]] auto union_internal_unsafe () -> internal*;
    [[nodiscard]] auto is_or_was_internal () const -> bool;

private:
    inline static constexpr uint32 MarkM   = 1U << (8 * sizeof(uint32) - 1);
    inline static constexpr uint32 UsedM   = 1U << (8 * sizeof(uint32) - 2);
    inline static constexpr uint32 LeafM   = 1U << (8 * sizeof(uint32) - 3);
    inline static constexpr uint32 RefsM   = ~(MarkM | UsedM | LeafM);
    inline static constexpr uint32 RefsMax = RefsM + 1;

private:
    alignas(internal) std::byte union_[sizeof(internal)];
    [[no_unique_address]] optional_member<Data> data_;
    node* next_;
    uint32 bits_;
};

template<class Data, degree D>
template<int32 N>
auto node<Data, D>::make_son_container(
    [[maybe_unused]] int32 const domain,
    [[maybe_unused]] degrees::fixed<N> const degree
)
    -> std::array<node*, as_usize(N)>
{
    return std::array<node*, as_usize(N)>();
}

template<class Data, degree D>
auto node<Data, D>::make_son_container(
    int32 const domain,
    [[maybe_unused]] degrees::mixed const degreeTag
)
    -> std::unique_ptr<node*[]>
{
    // Not supported yet.
    // return std::make_unique_for_overwrite<node*[]>(domain);
    return std::make_unique<node*[]>(as_usize(domain));
}

template<class Data, degree D>
node<Data, D>::node(int32 const value) : union_ {}, next_ {nullptr}, bits_ {LeafM | UsedM}
{
    std::construct_at(this->union_terminal(), value);
}

template<class Data, degree D>
node<Data, D>::node(int32 const index, son_container&& sons) :
    union_ {},
    next_ {nullptr},
    bits_ {UsedM}
{
    std::construct_at(this->union_internal(), std::move(sons), index);
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
node<Data, D>::~node()
{
    if (this->is_or_was_internal())
    {
        std::destroy_at(this->union_internal_unsafe());
    }
}

template<class Data, degree D>
template<class Foo>
requires(not std::is_void_v<Data>)
auto node<Data, D>::data() -> second_t<Foo, Data>&
{
    assert(this->is_used());
    return data_.member_;
}

template<class Data, degree D>
template<class Foo>
requires(not std::is_void_v<Data>)
auto node<Data, D>::data() const -> second_t<Foo, Data> const&
{
    assert(this->is_used());
    return data_.member_;
}

template<class Data, degree D>
auto node<Data, D>::get_next() const -> node*
{
    return next_;
}

template<class Data, degree D>
auto node<Data, D>::set_next(node* const next) -> void
{
    next_ = next;
}

template<class Data, degree D>
auto node<Data, D>::get_sons() const -> son_container const&
{
    return this->union_internal()->sons_;
}

template<class Data, degree D>
auto node<Data, D>::get_son(int32 const sonOrder) const -> node*
{
    return (this->union_internal()->sons_)[as_uindex(sonOrder)];
}

template<class Data, degree D>
auto node<Data, D>::set_sons(son_container sons) -> void
{
    using std::swap;
    swap(this->union_internal()->sons_, sons);
}

template<class Data, degree D>
auto node<Data, D>::get_value() const -> int32
{
    return this->union_terminal()->value_;
}

template<class Data, degree D>
auto node<Data, D>::is_internal() const -> bool
{
    return this->is_used() && not this->is_terminal();
}

template<class Data, degree D>
auto node<Data, D>::is_terminal() const -> bool
{
    return this->is_used() && (bits_ & LeafM);
}

template<class Data, degree D>
auto node<Data, D>::is_used() const -> bool
{
    return static_cast<bool>(bits_ & UsedM);
}

template<class Data, degree D>
auto node<Data, D>::set_unused() -> void
{
    bits_ &= ~UsedM;
}

template<class Data, degree D>
auto node<Data, D>::is_marked() const -> bool
{
    return static_cast<bool>(bits_ & MarkM);
}

template<class Data, degree D>
auto node<Data, D>::toggle_marked() -> void
{
    bits_ ^= MarkM;
}

template<class Data, degree D>
auto node<Data, D>::set_marked() -> void
{
    bits_ |= MarkM;
}

template<class Data, degree D>
auto node<Data, D>::set_notmarked() -> void
{
    bits_ &= ~MarkM;
}

template<class Data, degree D>
auto node<Data, D>::get_ref_count() const -> int32
{
    return static_cast<int32>(bits_ & RefsM);
}

template<class Data, degree D>
auto node<Data, D>::inc_ref_count() -> void
{
    assert(this->get_ref_count() < static_cast<int32>(RefsMax));
    ++bits_;
}

template<class Data, degree D>
auto node<Data, D>::dec_ref_count() -> void
{
    assert(this->get_ref_count() > 0);
    --bits_;
}

template<class Data, degree D>
auto node<Data, D>::get_index() const -> int32
{
    return this->union_internal()->index_;
}

template<class Data, degree D>
auto node<Data, D>::set_index(int32 const index) -> void
{
    this->union_internal()->index_ = index;
}

template<class Data, degree D>
auto node<Data, D>::union_internal() -> internal*
{
    assert(this->is_internal());
    return reinterpret_cast<internal*>(&union_);
}

template<class Data, degree D>
auto node<Data, D>::union_internal() const -> internal const*
{
    assert(this->is_internal());
    return reinterpret_cast<internal const*>(&union_);
}

template<class Data, degree D>
auto node<Data, D>::union_terminal() -> terminal*
{
    assert(this->is_terminal());
    return reinterpret_cast<terminal*>(&union_);
}

template<class Data, degree D>
auto node<Data, D>::union_terminal() const -> terminal const*
{
    assert(this->is_terminal());
    return reinterpret_cast<terminal const*>(&union_);
}

template<class Data, degree D>
auto node<Data, D>::union_internal_unsafe() -> internal*
{
    return reinterpret_cast<internal*>(&union_);
}

template<class Data, degree D>
auto node<Data, D>::is_or_was_internal() const -> bool
{
    return not static_cast<bool>(bits_ & LeafM);
}
} // namespace teddy

#endif