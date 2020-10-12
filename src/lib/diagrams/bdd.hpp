#ifndef MIX_DD_BDD_HPP
#define MIX_DD_BDD_HPP

#include "mdd.hpp"
#include "../utils/more_math.hpp"

#include <type_traits>

namespace mix::dd
{
    /**
        Ordered Binary Decision Diagram.
        @tparam VertexData - type of the data that will be stored in vertices of the diagram.
                Use void if you don't need to store any data.
        @tparam ArcData - type of the data that will be stored in arcs of the diagram.
                Use void if you don't need to store any data.
        @tparam Allocator - Allocator that is used by default to allocate vertices.
                std::allocator is used by default. You don't need to provide one.
    */
    template<class VertexData, class ArcData, class Allocator>
    class bdd : public mdd<VertexData, ArcData, 2, Allocator>
    {
    public:
        using base_t     = mdd<VertexData, ArcData, 2, Allocator>;
        using vertex_t = typename base_t::vertex_t;
        using arc_t    = typename base_t::arc_t;
        using log_t    = bool_t;

    public:
        /**
            Default constructed diagram is empty.
            Don't use diagram created this way.
        */
        bdd (Allocator const& alloc = Allocator {});

        /**
            Copy constructor.
        */
        bdd (bdd const& other);

        /**
            Move constructor.
        */
        bdd (bdd&& other) noexcept;

        /**
            @brief Swaps this diagram with the other one in constant time.
            @param other Diagram to swap with.
         */
        auto swap (bdd& other) noexcept -> void;

        /**
            Copy and move assign operator.
            See. https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
            @param rhs - diagram that is to be asigned into this one.
        */
        auto operator= (bdd rhs) noexcept -> bdd&;

        /**
            Creates copy of this diagram using the copy constructor.

            @return deep copy of this diagram.
         */
        auto clone () const -> bdd;

        /**
            Creates new diagrams by moving this diagram into to new one.
            This diagram is left empty.

            @return new diagram with content of this diagram.
         */
        auto move () -> bdd;

        /**
            @param  rhs   - other diagram that should be compared with this.
            @return true  if this and rhs diagrams represent the same function.
                    false otherwise
        */
        auto operator== (bdd const& rhs) const -> bool;

        /**
            @param  rhs   - other diagram that should be compared with this.
            @return false if this and rhs diagrams represents the same function.
                    true  otherwise.
        */
        auto operator!= (bdd const& rhs) const -> bool;

        /**
            @tparam VariableValues - type that stores values of the variables.
            @tparam OutputIt - output iterator that accepts VariableValues.
            @tparam SetVarVal - functor that sets the value of i-th variable 
                    in an instance of VariableValues. std::bitset<N> and integral
                    types are supported by default.
            @param  out output iterator.
        */
        template< class VariableValues
                , class OutputIt
                , class SetVarVal = set_var_val<VariableValues> >
        auto satisfy_all (OutputIt out) const -> void;

        /**
            Calculates the size size of the satisfying set of the function.
            Uses data member of vertices to store intermediated results if possible.
            It is possible when VertexData is floating or at least 32bit integral type.
            Otherwise it uses a map which might be a bit slower.

            @return size of the satisfying set of the function.
        */
        auto satisfy_count () -> std::size_t;

        /**
            Calculates the size size of the satisfying set of the function.
            Uses map to store intermediated results.

            @return size of the satisfying set of the function.
         */
        auto satisfy_count () const -> std::size_t;

        /**
            @return Pointer to the leaf vertex that represents true value.
                    nullptr if there is no such vertex.
         */
        auto true_leaf   () -> vertex_t*;

        /**
            @return Pointer to the leaf vertex that represents false value.
                    nullptr if there is no such vertex.
         */
        auto false_leaf  () -> vertex_t*;

    private:
        friend class bdd_manipulator<VertexData, ArcData, Allocator>;
        friend class bdd_creator<VertexData, ArcData, Allocator>;
        friend class bdd_reliability<VertexData, ArcData, Allocator>;
        friend class mdd_manipulator<VertexData, ArcData, 2, Allocator>;
        friend class mdd_creator<VertexData, ArcData, 2, Allocator>;

    private:
        using leaf_val_map = typename base_t::leaf_val_map;
        using manager_t    = typename base_t::manager_t;

    private:
        bdd ( vertex_t* const  root
            , leaf_val_map     leafToVal
            , Allocator const& alloc );

        template< class VariableValues
                , class InsertIterator
                , class SetVarVal = set_var_val<VariableValues> >
        auto satisfy_all_step ( index_t   const i
                              , vertex_t* const v
                              , VariableValues& xs
                              , InsertIterator& out ) const -> void;

        static auto are_equal ( vertex_t* const v1
                              , vertex_t* const v2
                              , bdd const&      d1
                              , bdd const&      d2 ) -> bool;
    };

    template<class VertexData, class ArcData, class Allocator>
    bdd<VertexData, ArcData, Allocator>::bdd
        (Allocator const& alloc) :
        base_t {alloc}
    {
    }

    template<class VertexData, class ArcData, class Allocator>
    bdd<VertexData, ArcData, Allocator>::bdd(bdd const& other) :
        base_t {other}
    {
    }

    template<class VertexData, class ArcData, class Allocator>
    bdd<VertexData, ArcData, Allocator>::bdd
        (bdd&& other) noexcept :
        base_t {std::move(other)}
    {
    }

    template<class VertexData, class ArcData, class Allocator>
    bdd<VertexData, ArcData, Allocator>::bdd
        ( vertex_t* const  root
        , leaf_val_map     leafToVal
        , Allocator const& alloc ) :
        base_t {root, std::move(leafToVal), alloc}
    {
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd<VertexData, ArcData, Allocator>::swap
        (bdd& other) noexcept -> void
    {
        base_t::swap(other);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd<VertexData, ArcData, Allocator>::operator= 
        (bdd rhs) noexcept -> bdd&
    {
        base_t::operator= (std::move(rhs));
        return *this;
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd<VertexData, ArcData, Allocator>::operator==
        (const bdd& rhs) const -> bool 
    {
        if (base_t::root_ == rhs.root_)
        {
            // Catches comparison with self and also case when both diagrams are empty.
            return true;
        }

        if (!base_t::root_ || ! rhs.root_)
        {
            // Case when one of the roots is null.
            return false;
        }

        if (this->variable_count() != rhs.variable_count())
        {
            // Different number of variables. Can't be equal.
            return false;
        }

        // Compare trees.
        return bdd::are_equal(base_t::root_, rhs.root_, *this, rhs);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd<VertexData, ArcData, Allocator>::operator!=
        (bdd const& rhs) const -> bool 
    {
        return ! (*this == rhs);
    }

    template<class VertexData, class ArcData, class Allocator>
    template<class VariableValues, class OutputIt, class SetVarVal>
    auto bdd<VertexData, ArcData, Allocator>::satisfy_all
        (OutputIt out) const -> void
    {
        auto xs = VariableValues {};
        this->satisfy_all_step(0, base_t::root_, xs, out);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd<VertexData, ArcData, Allocator>::satisfy_count
        () -> std::size_t
    {
        return base_t::satisfy_count(1);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd<VertexData, ArcData, Allocator>::satisfy_count
        () const -> std::size_t
    {
        return base_t::satisfy_count(1);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd<VertexData, ArcData, Allocator>::clone
        () const -> bdd
    {
        return bdd {*this};
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd<VertexData, ArcData, Allocator>::move
        () -> bdd
    {
        return bdd {std::move(*this)};
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd<VertexData, ArcData, Allocator>::true_leaf
        () -> vertex_t*
    {
        return base_t::get_leaf(1u);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd<VertexData, ArcData, Allocator>::false_leaf
        () -> vertex_t*
    {
        return base_t::get_leaf(0u);
    }

    template<class VertexData, class ArcData, class Allocator>
    template<class VariableValues, class InsertIterator, class SetVarVal>
    auto bdd<VertexData, ArcData, Allocator>::satisfy_all_step 
        ( index_t   const i
        , vertex_t* const v
        , VariableValues& xs
        , InsertIterator& out ) const -> void
    {
        auto const set_var = SetVarVal {};

        if (0 == this->value(v) || is_undefined<2>(this->value(v)) )
        {
            return;
        }
        else if (i == this->leaf_index() && 1 == this->value(v))
        {
            out = xs;
            return;
        }
        else if (v->get_index() > i)
        {
            set_var(xs, i, 0);
            satisfy_all_step(i + 1, v, xs, out);
            set_var(xs, i, 1);
            satisfy_all_step(i + 1, v, xs, out);
        }
        else
        {
            set_var(xs, i, 0);
            satisfy_all_step(i + 1, v->get_son(0), xs, out);
            set_var(xs, i, 1);
            satisfy_all_step(i + 1, v->get_son(1), xs, out);
        }
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd<VertexData, ArcData, Allocator>::are_equal 
        ( vertex_t* const v1
        , vertex_t* const v2
        , const bdd&      d1
        , const bdd&      d2 ) -> bool
    {
        if (v1->get_index() != v2->get_index())
        {
            // Different indices, can't be equal.
            return false;
        }

        if (d1.is_leaf(v1) != d2.is_leaf(v2))
        {
            // One of them is not leaf, can't be equal.
            return false;
        }

        if (d1.is_leaf(v1))
        {
            // Both are leafs so their values must match.
            return d1.leafToVal.at(v1) == d2.leafToVal.at(v2);
        }

        auto const equalOnTheLeft = are_equal(v1->get_son(0), v2->get_son(0), d1, d2);
        if (! equalOnTheLeft)
        {
            // Left subtrees aren't equal => can't be equal.
            return false;
        }

        auto equalOnTheRight = are_equal(v1->get_son(1), v2->get_son(1), d1, d2);
        if (! equalOnTheRight)
        {
            // Right subtrees aren't equal => can't be equal.
            return false;
        }

        // A this point, the trees are equal.
        return true;
    }
}

#endif