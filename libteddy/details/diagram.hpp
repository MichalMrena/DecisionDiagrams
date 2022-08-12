#ifndef LIBTEDDY_DETAILS_DIAGRAM_HPP
#define LIBTEDDY_DETAILS_DIAGRAM_HPP

#include <cassert>
#include <libteddy/details/node.hpp>
#include <libteddy/details/node_manager.hpp>
#include <utility>

namespace teddy
{
    template<class Data, degree Deg, domain Dom>
    class diagram_manager;

    template<degree Deg, domain Dom>
    class reliability_manager;

    /**
     *  \class diagram
     *  \brief Cheap wrapper for the internal diagram node type.
     *
     *  Instance of the diagram holds pointer to an internal node. Therefore,
     *  it is a cheap value type. Multiple diagrams can point to a same node
     *  i.e. represent the same function.
     */
    template<class Data, degree D>
    class diagram
    {
    public:
        template<class Da, degree De, domain Do>
        friend class diagram_manager;

        template<degree De, domain Do>
        friend class reliability_manager;

    private:
        using node_t = node<Data, D>;

    public:
        /**
         *  \brief Default constructed diagram. Points to no node
         *  and should not be used.
         *
         *  Technically, this constructor does not need to exists at all but for
         *  the library user it might be useful to create e.g. vector
         *  of empty diagrams and assign them later.
         */
        diagram();

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
        auto operator=(diagram other) -> diagram&;

        /**
         *  \brief Swaps pointers in this and other diagram.
         *  \param other Diagram to be swapped with this one.
         */
        auto swap(diagram& other) -> void;

        /**
         *  \brief Compares node pointers in this and other diagram.
         *  \param other Diagram to be compared with this one.
         *  \return true iif diagrams represent the same function.
         */
        auto equals(diagram other) const -> bool;

    public:
        explicit diagram(node_t*);
        auto get_root() const -> node_t*;

    private:
        node_t* root_ {nullptr};
    };

    /**
     *  \brief Swaps pointer in the two diagrams.
     *  \param lhs First diagram.
     *  \param rhs Second diagram.
     */
    template<class Data, degree D>
    auto swap(diagram<Data, D>& lhs, diagram<Data, D>& rhs) -> void
    {
        lhs.swap(rhs);
    }

    /**
     *  \brief Compares two diagrams.
     *  \param lhs First diagram.
     *  \param rhs Second diagram.
     *  \return true iif diagrams represent the same function.
     */
    template<class Data, degree D>
    auto operator==(diagram<Data, D> lhs, diagram<Data, D> rhs) -> bool
    {
        return lhs.equals(rhs);
    }

    /**
     *  \brief Compares two diagrams.
     *  \param lhs First diagram.
     *  \param rhs Second diagram.
     *  \return true iif diagrams represent the same function.
     */
    template<class Data, degree D>
    auto equals(diagram<Data, D> lhs, diagram<Data, D> rhs) -> bool
    {
        return lhs.equals(rhs);
    }

    template<class Data, degree D>
    diagram<Data, D>::diagram() : root_(nullptr)
    {
    }

    template<class Data, degree D>
    diagram<Data, D>::diagram(node_t* const r)
        : root_(id_set_notmarked(id_inc_ref_count(r)))
    {
    }

    template<class Data, degree D>
    diagram<Data, D>::diagram(diagram const& d)
        : root_(id_inc_ref_count(d.get_root()))
    {
    }

    template<class Data, degree D>
    diagram<Data, D>::diagram(diagram&& d) noexcept
        : root_(std::exchange(d.root_, nullptr))
    {
    }

    template<class Data, degree D>
    diagram<Data, D>::~diagram()
    {
        if (root_)
        {
            root_->dec_ref_count();
        }
    }

    template<class Data, degree D>
    auto diagram<Data, D>::get_root() const -> node_t*
    {
        assert(root_);
        return root_;
    }

    template<class Data, degree D>
    auto diagram<Data, D>::operator=(diagram d) -> diagram&
    {
        d.swap(*this);
        return *this;
    }

    template<class Data, degree D>
    auto diagram<Data, D>::swap(diagram& d) -> void
    {
        std::swap(root_, d.root_);
    }

    template<class Data, degree D>
    auto diagram<Data, D>::equals(diagram d) const -> bool
    {
        return root_ == d.get_root();
    }
} // namespace teddy

#endif