#ifndef MIX_DD_DIAGRAM_HPP
#define MIX_DD_DIAGRAM_HPP

#include "node.hpp"
#include "node_manager.hpp"
#include <cassert>
#include <utility>

namespace teddy
{
    template<class Data, degree Deg, domain Dom>
    class diagram_manager;

    template<class Data, degree D>
    class diagram
    {
    public:
        template<class Da, degree De, domain Do>
        friend class diagram_manager;

    public:
        using node_t = node<Data, D>;

    public:
        diagram ();
        explicit diagram (node_t*);
        diagram (diagram const&);
        diagram (diagram&&) noexcept;
        ~diagram();

        auto operator= (diagram)  -> diagram&;
        auto swap      (diagram&) -> void;
        auto equals    (diagram const&) const -> bool;

    private:
        auto get_root () const -> node_t*;

    private:
        node_t* root_ {nullptr};
    };

    template<class Data, degree D>
    auto swap (diagram<Data, D>& l, diagram<Data, D>& r)
    {
        l.swap(r);
    }

    template<class Data, degree D>
    auto equals (diagram<Data, D> const& l, diagram<Data, D> const& r)
    {
        return l.equals(r);
    }

    template<class Data, degree D>
    diagram<Data, D>::diagram
        () :
        root_ (nullptr)
    {
    }

    template<class Data, degree D>
    diagram<Data, D>::diagram
        (node_t* const r) :
        root_ ([](auto const n){ n->inc_ref_count(); return n; }(r))
    {
    }

    template<class Data, degree D>
    diagram<Data, D>::diagram
        (diagram const& d) :
        diagram (d.get_root())
    {
    }

    template<class Data, degree D>
    diagram<Data, D>::diagram
        (diagram&& d) noexcept :
        root_ (std::exchange(d.root_, nullptr))
    {
    }

    template<class Data, degree D>
    diagram<Data, D>::~diagram
        ()
    {
        if (root_)
        {
            root_->dec_ref_count();
        }
    }

    template<class Data, degree D>
    auto diagram<Data, D>::get_root
        () const -> node_t*
    {
        assert(root_);
        return root_;
    }

    template<class Data, degree D>
    auto diagram<Data, D>::operator=
        (diagram d) -> diagram&
    {
        d.swap(*this);
        return *this;
    }

    template<class Data, degree D>
    auto diagram<Data, D>::swap
        (diagram& d) -> void
    {
        std::swap(root_, d.root_);
    }

    template<class Data, degree D>
    auto diagram<Data, D>::equals
        (diagram const& d) const -> bool
    {
        return root_ == d.get_root();
    }
}

#endif