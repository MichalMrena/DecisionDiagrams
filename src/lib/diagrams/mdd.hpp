#ifndef MIX_DD_MDD_HPP
#define MIX_DD_MDD_HPP

#include "typedefs.hpp"
#include "graph.hpp"
#include "vertex_manager.hpp"

#include <cstddef>
#include <utility>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    class mdd
    {
    public:
        using vertex_t = vertex<VertexData, ArcData, P>;

    public:
        mdd  ();
        mdd  (vertex_t* root);
        mdd  (mdd&& other);
        ~mdd ();
        explicit mdd (mdd const& other);

        auto operator= (mdd rhs)  -> mdd&;
        auto swap      (mdd& rhs) -> void;
        auto get_root  () const   -> vertex_t*;

    private:
        using manager_t = vertex_manager<VertexData, ArcData, P>;

    private:
        vertex_t* root_;
    };

    template<class VertexData, class ArcData, std::size_t P>
    mdd<VertexData, ArcData, P>::mdd
        () :
        mdd {nullptr}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    mdd<VertexData, ArcData, P>::mdd
        (vertex_t* root) :
        root_ {manager_t::inc_ref_count(root)}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    mdd<VertexData, ArcData, P>::mdd
        (mdd&& other) :
        root_ {std::exchange(other.root_, nullptr)}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    mdd<VertexData, ArcData, P>::mdd
        (mdd const& other) :
        root_ {manager_t::inc_ref_count(other.root_)}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    mdd<VertexData, ArcData, P>::~mdd
        ()
    {
        manager_t::dec_ref_count(root_);
        root_ = nullptr;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd<VertexData, ArcData, P>::operator=
        (mdd rhs) -> mdd&
    {
        rhs.swap(*this);
        return *this;
    }
}

#endif