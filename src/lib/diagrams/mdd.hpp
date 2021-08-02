#ifndef MIX_DD_MDD_HPP
#define MIX_DD_MDD_HPP

namespace teddy
{
    template<class VertexData, class ArcData, std::size_t P>
    class mdd_manager;

    template<class VertexData, class ArcData>
    class bdd_manager;

    template<class VertexData, class ArcData, std::size_t P>
    class mdd
    {
    public:
        friend class mdd_manager<VertexData, ArcData, P>;
        friend class bdd_manager<VertexData, ArcData>;

    public:
        using vertex_t = vertex<VertexData, ArcData, P>;

    public:
        mdd  ();
        ~mdd ();
        mdd  (mdd&& other) noexcept;
        mdd  (mdd const& other);
        explicit mdd (vertex_t* root);

        auto operator= (mdd rhs)              -> mdd&;
        auto swap      (mdd& rhs) noexcept    -> void;
        auto equals    (mdd const& rhs) const -> bool;

    private:
        auto get_root () const -> vertex_t*;

    private:
        vertex_t* root_;
    };

    template<class VertexData, class ArcData, std::size_t P>
    mdd<VertexData, ArcData, P>::mdd
        () :
        root_ {nullptr}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    mdd<VertexData, ArcData, P>::mdd
        (vertex_t* root) :
        root_ {[](auto const v){ v->inc_ref_count(); return v; }(root) }
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    mdd<VertexData, ArcData, P>::mdd
        (mdd&& other) noexcept :
        root_ {std::exchange(other.root_, nullptr)}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    mdd<VertexData, ArcData, P>::mdd
        (mdd const& other) :
        root_ {[](auto const v){ v->inc_ref_count(); return v; }(other.root_)}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    mdd<VertexData, ArcData, P>::~mdd
        ()
    {
        if (root_)
        {
            root_->dec_ref_count();
            root_ = nullptr;
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd<VertexData, ArcData, P>::operator=
        (mdd rhs) -> mdd&
    {
        rhs.swap(*this);
        return *this;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd<VertexData, ArcData, P>::swap
        (mdd& rhs) noexcept -> void
    {
        std::swap(root_, rhs.root_);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd<VertexData, ArcData, P>::equals
        (mdd const& rhs) const -> bool
    {
        return this->get_root() == rhs.get_root();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd<VertexData, ArcData, P>::get_root
        () const -> vertex_t*
    {
        return root_;
    }
}

#endif