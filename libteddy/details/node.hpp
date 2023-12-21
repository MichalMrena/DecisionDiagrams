#ifndef LIBTEDDY_DETAILS_NODE_HPP
#define LIBTEDDY_DETAILS_NODE_HPP

#include <libteddy/details/tools.hpp>
#include <libteddy/details/types.hpp>

#include <cassert>
#include <cstdlib>

namespace teddy
{
namespace degrees
{
    /**
     *  \brief Marks that different nodes can have different number of sons
     */
    struct mixed
    {
        // Just a dummy value to simplify ifs, this one should be never used
        static int32 constexpr value = 1;
    };

    /**
     *  \brief Marks that all nodes have the same number of sons
     */
    template<int32 N>
    struct fixed
    {
        static_assert(N > 1);
        static int32 constexpr value = N;
    };

    /**
     *  \brief Trait that checks if a degree (T) is fixed
     */
    template<class T>
    struct is_fixed
    {
    };

    template<>
    struct is_fixed<mixed>
    {
        static bool constexpr value = false;
    };

    template<int32 N>
    struct is_fixed<fixed<N>>
    {
        static bool constexpr value = true;
    };

    /**
     *  \brief Trait that checks if a degree (T) is mixed
     */
    template<class T>
    struct is_mixed
    {
    };

    template<int32 N>
    struct is_mixed<fixed<N>>
    {
        static bool constexpr value = false;
    };

    template<>
    struct is_mixed<mixed>
    {
        static bool constexpr value = true;
    };
} // namespace degrees

namespace details
{
    template<std::size_t Count>
    struct bytes
    {
        char bytes_[Count];
    };

    template<>
    struct bytes<0>
    {
    };
} // namespace details

template<class Data, class Degree>
class node;

namespace sons
{
    /**
     *  \brief Wrapper around statically allocated array of node pointers
     */
    template<class Data, class Degree>
    class fixed
    {
    public:
        using node_t = node<Data, Degree>;

        auto operator[] (int64 const index) -> node_t*&
        {
            return sons_[index];
        }

        auto operator[] (int64 const index) const -> node_t* const&
        {
            return sons_[index];
        }

    private:
        node_t* sons_[Degree::value];
    };

    /**
     *  \brief RAII wrapper around dynamically allocated array of node pointers
     */
    template<class Data, class Degree>
    class mixed
    {
    public:
        using node_t = node<Data, Degree>;

        mixed(int32 const domain) :
            sons_(static_cast<node_t**>(
                std::malloc(as_usize(domain) * sizeof(node_t*))
            ))
        {
        }

        mixed(mixed const&) = delete;

        mixed(mixed&& other) : sons_(utils::exchange(other.sons_, nullptr))
        {
        }

        ~mixed()
        {
            std::free(sons_);
        }

        auto operator= (mixed&& other) -> mixed&
        {
            if (this != &other) [[likely]]
            {
                std::free(sons_);
            }
            sons_ = utils::exchange(other.sons_, nullptr);
            return *this;
        }

        auto operator[] (int64 const index) -> node_t*&
        {
            return sons_[index];
        }

        auto operator[] (int64 const index) const -> node_t* const&
        {
            return sons_[index];
        }

    private:
        node_t** sons_;
    };

    /**
     *  \brief Factory function for fixed son container
     */
    template<class Data, int32 N>
    auto make_son_container (int32, degrees::fixed<N>)
    {
        return fixed<Data, degrees::fixed<N>>();
    }

    /**
     *  \brief Factory function for mixed son container
     */
    template<class Data>
    auto make_son_container (int32 const domain, degrees::mixed)
    {
        return mixed<Data, degrees::mixed>(domain);
    }
} // namespace sons

/**
 *  \brief TODO
 */
template<class Data, class Degree>
class node
{
public:
    using son_container
        = decltype(sons::make_son_container<Data, Degree>(int32(), Degree()));

public:
    /**
     *  \brief Factory function for son_container
     */
    static auto make_son_container (int32 const domain) -> son_container
    {
        return sons::make_son_container<Data, Degree>(domain, Degree());
    }

public:
    /**
     *  \brief Constructs node as terminal
     */
    explicit node(int32 value) :
        terminal_ {value},
        next_ {nullptr},
        bits_ {LeafM | UsedM}
    {
    }

    /**
     *  \brief Constructs node as internal
     */
    node(int32 index, son_container sons) :
        internal_ {TEDDY_MOVE(sons), index},
        next_ {nullptr},
        bits_ {UsedM}
    {
    }

    /**
     *  \brief Trivial destructor if sons are fixed
     */
    ~node() = default;

    /**
     *  \brief Non-Trivial destructor if sons are mixed
     */
    ~node()
    requires(degrees::is_mixed<Degree>::value)
    {
        if constexpr (degrees::is_mixed<Degree>::value)
        {
            if (this->is_or_was_internal())
            {
                internal_.sons_.~son_container();
            }
        }
    }

    node()                       = delete;
    node(node const&)            = delete;
    node(node&&)                 = delete;
    auto operator= (node const&) = delete;
    auto operator= (node&&)      = delete;

    // TODO get_data_as<double>() -> double&
    //      get_data_as<expr>() -> expr&
    // asi uz nebude treba Foo
    template<class Foo = void>
    requires(not utils::is_void<Data>::value)
    auto get_data () -> utils::second_t<Foo, Data>&
    {
        assert(this->is_used());
        return data_.member_;
    }

    template<class Foo = void>
    requires(not utils::is_void<Data>::value)
    auto get_data () const -> utils::second_t<Foo, Data> const&
    {
        assert(this->is_used());
        return data_.member_;
    }

    [[nodiscard]]
    auto is_internal () const -> bool
    {
        return this->is_used() && (bits_ & LeafM);
    }

    [[nodiscard]]
    auto is_terminal () const -> bool
    {
        return this->is_used() && (bits_ & LeafM);
    }

    [[nodiscard]]
    auto is_used () const -> bool
    {
        return static_cast<bool>(bits_ & UsedM);
    }

    [[nodiscard]]
    auto is_marked () const -> bool
    {
        return static_cast<bool>(bits_ & MarkM);
    }

    [[nodiscard]]
    auto get_next () const -> node*
    {
        return next_;
    }

    [[nodiscard]]
    auto get_ref_count () const -> int32
    {
        return static_cast<int32>(bits_ & RefsM);
    }

    [[nodiscard]]
    auto get_index () const -> int32
    {
        assert(this->is_internal());
        return internal_.index_;
    }

    [[nodiscard]]
    auto get_sons () const -> son_container const&
    {
        assert(this->is_internal());
        return internal_.sons_;
    }

    [[nodiscard]]
    auto get_son (int32 sonOrder) const -> node*
    {
        assert(this->is_internal());
        return internal_.sons_[sonOrder];
    }

    [[nodiscard]]
    auto get_value () const -> int32
    {
        assert(this->is_terminal());
        return terminal_.value_;
    }

    auto set_next (node* next) -> void
    {
        next_ = next;
    }

    auto set_unused () -> void
    {
        bits_ &= ~UsedM;
    }

    auto set_marked () -> void
    {
        bits_ |= MarkM;
    }

    auto set_notmarked () -> void
    {
        bits_ &= ~MarkM;
    }

    auto set_index (int32 index) -> void
    {
        assert(this->is_internal());
        internal_.index_ = index;
    }

    auto set_sons (son_container sons) -> void
    {
        assert(this->is_internal());
        internal_.sons_ = TEDDY_MOVE(sons);
    }

    auto toggle_marked () -> void
    {
        bits_ ^= MarkM;
    }

    auto inc_ref_count () -> void
    {
        assert(this->get_ref_count() < static_cast<int32>(RefsMax));
        ++bits_;
    }

    auto dec_ref_count () -> void
    {
        assert(this->get_ref_count() > 0);
        --bits_;
    }

private:
    struct internal
    {
        son_container sons_;
        int32 index_;
    };

    struct terminal
    {
        int32 value_;
    };

private:
    [[nodiscard]]
    auto is_or_was_internal () const -> bool
    {
        return not static_cast<bool>(bits_ & LeafM);
    }

private:
    static uint32 constexpr MarkM   = 1U << (8 * sizeof(uint32) - 1);
    static uint32 constexpr UsedM   = 1U << (8 * sizeof(uint32) - 2);
    static uint32 constexpr LeafM   = 1U << (8 * sizeof(uint32) - 3);
    static uint32 constexpr RefsM   = ~(MarkM | UsedM | LeafM);
    static uint32 constexpr RefsMax = RefsM + 1;

private:
    union
    {
        internal internal_;
        terminal terminal_;
    };

    [[no_unique_address]]
    utils::optional_member<Data> data_;

    node* next_;

    /*
     *  1b  -> is marked flag   (highest bit)
     *  1b  -> is used flag
     *  1b  -> is leaf flag
     *  29b -> reference count  (lowest bits)
     */
    uint32 bits_;
};
} // namespace teddy

#endif