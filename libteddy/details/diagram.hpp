#ifndef LIBTEDDY_DETAILS_DIAGRAM_HPP
#define LIBTEDDY_DETAILS_DIAGRAM_HPP

#include <libteddy/details/node.hpp>
#include <libteddy/details/node_manager.hpp>

#include <cstddef>
#include <functional>
#include <utility>

namespace teddy
{
/**
 *  \class diagram
 *  \brief Cheap wrapper for the internal diagram node type.
 *
 *  Instance of the diagram holds pointer to an internal node. Therefore,
 *  it is a cheap value type. Multiple diagrams can point to a same node
 *  i.e. represent the same function.
 */
template<class Data, class Degree>
class diagram
{
public:
    using node_t = node<Data, Degree>;

public:
    /**
     *  \brief Default constructed diagram. Points to no node
     *  and should not be used.
     *
     *  Technically, this constructor does not need to exists at all but for
     *  the library user it might be useful to create e.g. vector
     *  of empty diagrams and assign them later.
     */
    diagram() = default;

    /**
     *  \brief Wraps internal node representation.
     *  You should probably don't use this one unless you know what you are
     *  doing.
     *  \param root root node of the diagram
     */
    diagram(node_t* root);

    /**
     *  \brief Cheap copy constructor.
     *  \param other Diagram to be copied
     */
    diagram(diagram const& other);

    /**
     *  \brief Cheap move constructor.
     *  \param other Diagram to be moved from.
     */
    diagram(diagram&& other) noexcept;

    /**
     *  \brief Destructor. Ensures correct reference counting
     *  using RAII pattern.
     */
    ~diagram();

    /**
     *  \brief Assigns pointer from the other diagram.
     *  \param other Diagram to be assigned into this one.
     *  \return Reference to this diagram.
     */
    auto operator= (diagram other) -> diagram&;

    /**
     *  \brief Swaps pointers in this and other diagram.
     *  \param other Diagram to be swapped with this one.
     */
    auto swap (diagram& other) -> void;

    /**
     *  \brief Compares node pointers in this and other diagram.
     *  \param other Diagram to be compared with this one.
     *  \return true iif diagrams represent the same function.
     */
    auto equals (diagram const& other) const -> bool;

    /**
     *  \brief Returns pointer to internal node type.
     *  You should probably don't use this one unless you know what you are
     *  doing.
     *  \return Pointer to node root node of the diagram.
     */
    auto unsafe_get_root () const -> node_t*;

private:
    node_t* root_ {nullptr};
};

/**
 *  \brief Swaps pointer in the two diagrams.
 *  \param lhs First diagram.
 *  \param rhs Second diagram.
 */
template<class Data, class Degree>
auto swap (diagram<Data, Degree>& lhs, diagram<Data, Degree>& rhs) -> void
{
    lhs.swap(rhs);
}

/**
 *  \brief Compares two diagrams.
 *  \param lhs First diagram.
 *  \param rhs Second diagram.
 *  \return true iif diagrams represent the same function.
 */
template<class Data, class Degree>
auto operator== (
    diagram<Data, Degree> const& lhs,
    diagram<Data, Degree> const& rhs
) -> bool
{
    return lhs.equals(rhs);
}

/**
 *  \brief Compares two diagrams.
 *  \param lhs First diagram.
 *  \param rhs Second diagram.
 *  \return true iif diagrams represent the same function.
 */
template<class Data, class Degree>
auto equals (diagram<Data, Degree> lhs, diagram<Data, Degree> rhs) -> bool
{
    return lhs.equals(rhs);
}

template<class Data, class Degree>
diagram<Data, Degree>::diagram(node_t* const root) :
    root_(id_set_notmarked(id_inc_ref_count(root)))
{
}

template<class Data, class Degree>
diagram<Data, Degree>::diagram(diagram const& other) :
    root_(id_inc_ref_count(other.root_))
{
}

template<class Data, class Degree>
diagram<Data, Degree>::diagram(diagram&& other) noexcept :
    root_(std::exchange(other.root_, nullptr))
{
}

template<class Data, class Degree>
diagram<Data, Degree>::~diagram()
{
    if (root_)
    {
        root_->dec_ref_count();
    }
}

template<class Data, class Degree>
auto diagram<Data, Degree>::operator= (diagram other) -> diagram&
{
    other.swap(*this);
    return *this;
}

template<class Data, class Degree>
auto diagram<Data, Degree>::swap(diagram& other) -> void
{
    std::swap(root_, other.root_);
}

template<class Data, class Degree>
auto diagram<Data, Degree>::equals(diagram const& other) const -> bool
{
    return root_ == other.unsafe_get_root();
}

template<class Data, class Degree>
auto diagram<Data, Degree>::unsafe_get_root() const -> node_t*
{
    return root_;
}
} // namespace teddy

namespace std
{
template<class Data, class Degree>
struct hash<teddy::diagram<Data, Degree>>
{
    [[nodiscard]] auto operator() (teddy::diagram<Data, Degree> const& diagram
    ) const noexcept -> std::size_t
    {
        return std::hash<decltype(diagram.unsafe_get_root())>(
        )(diagram.unsafe_get_root());
    }
};

template<class Data, class Degree>
struct equal_to<teddy::diagram<Data, Degree>>
{
    [[nodiscard]] auto operator() (
        teddy::diagram<Data, Degree> const& lhs,
        teddy::diagram<Data, Degree> const& rhs
    ) const noexcept -> bool
    {
        return lhs.equals(rhs);
    }
};
} // namespace std

#endif