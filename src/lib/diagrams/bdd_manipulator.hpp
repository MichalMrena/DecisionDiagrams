#ifndef MIX_DD_BDD_MANIPULATOR_
#define MIX_DD_BDD_MANIPULATOR_

#include "mdd_manipulator.hpp"
#include "operators.hpp"

#include <algorithm>

namespace mix::dd
{
    template<class VertexData, class ArcData, class Allocator>
    class bdd_manipulator : public mdd_manipulator<VertexData, ArcData, 2, Allocator>
    {
    public:
        using bdd_t    = bdd<VertexData, ArcData, Allocator>;
        using vertex_t = typename bdd_t::vertex_t;
        using arc_t    = typename bdd_t::arc_t;

    public:
        explicit bdd_manipulator (Allocator const& alloc = Allocator {});

        auto restrict_var (bdd_t&  diagram, index_t const i, bool_t const val) -> bdd_t&;
        auto restrict_var (bdd_t&& diagram, index_t const i, bool_t const val) -> bdd_t;

        /**
            Performs logical not on the @p diagram with constant complexity.
            Operation negates referenced diagram and doesn't created new one.
            @return @p diagram
         */
        auto negate (bdd_t&  diagram) -> bdd_t&;

        /**
            Performs logical not on the @p diagram with constant complexity.
            @return new negated move constructed diagram.
         */
        auto negate (bdd_t&& diagram) -> bdd_t;

    private:
        using base = mdd_manipulator<VertexData, ArcData, 2, Allocator>;
    };

    /**
        Performs apply AND.
     */
    template<class VertexData, class ArcData, class Allocator>
    auto operator&& ( bdd<VertexData, ArcData, Allocator> const& lhs
                    , bdd<VertexData, ArcData, Allocator> const& rhs ) -> bdd<VertexData, ArcData, Allocator>;
    
    /**
        Performs apply AND.
     */
    template<class VertexData, class ArcData, class Allocator>
    auto operator* ( bdd<VertexData, ArcData, Allocator> const& lhs
                   , bdd<VertexData, ArcData, Allocator> const& rhs ) -> bdd<VertexData, ArcData, Allocator>;

    /**
        Performs apply OR.
     */
    template<class VertexData, class ArcData, class Allocator>
    auto operator|| ( bdd<VertexData, ArcData, Allocator> const& lhs
                    , bdd<VertexData, ArcData, Allocator> const& rhs ) -> bdd<VertexData, ArcData, Allocator>;

    /**
        Performs apply OR.
     */
    template<class VertexData, class ArcData, class Allocator>
    auto operator+ ( bdd<VertexData, ArcData, Allocator> const& lhs
                   , bdd<VertexData, ArcData, Allocator> const& rhs ) -> bdd<VertexData, ArcData, Allocator>;
    
    /**
        Performs apply XOR.
     */
    template<class VertexData, class ArcData, class Allocator>
    auto operator^ ( bdd<VertexData, ArcData, Allocator> const& lhs
                   , bdd<VertexData, ArcData, Allocator> const& rhs ) -> bdd<VertexData, ArcData, Allocator>;
    
    /**
        Performs negate.
     */
    template<class VertexData, class ArcData, class Allocator>
    auto operator! ( bdd<VertexData, ArcData, Allocator>& lhs ) -> bdd<VertexData, ArcData, Allocator>&;
    
    /**
        Performs negate.
     */
    template<class VertexData, class ArcData, class Allocator>
    auto operator! ( bdd<VertexData, ArcData, Allocator>&& lhs ) -> bdd<VertexData, ArcData, Allocator>;

// definitions:

    template<class VertexData, class ArcData, class Allocator>
    bdd_manipulator<VertexData, ArcData, Allocator>::bdd_manipulator
        (Allocator const& alloc) :
        base {alloc}
    {
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_manipulator<VertexData, ArcData, Allocator>::restrict_var
        (bdd_t& diagram, const index_t i, const bool_t val) -> bdd_t&
    {
        if (i >= diagram.variable_count())
        {
            return diagram;
        }

        auto const oldVertices = diagram.template fill_container< std::set<vertex_t*> >();

        // "skip" all vertices with given index
        diagram.traverse_pre(diagram.root_, [i, val, &diagram](vertex_t* const v)
        {
            // if (v->is_leaf())
            if (diagram.is_leaf(v))
            {
                return;
            }

            if (! diagram.is_leaf(v->son(0)) && i == v->son(0)->index)
            {
                v->son(0) = v->son(0)->son(val);
            }

            if (! diagram.is_leaf(v->son(1)) && i == v->son(1)->index)
            {
                v->son(1) = v->son(1)->son(val);
            }
        });   

        // possibly change the root
        if (i == diagram.root_->index)
        {
            diagram.root_ = diagram.root_->son(val);
        }

        // identify now unreachable vertices
        auto const newVertices   = diagram.template fill_container< std::set<vertex_t*> >();
        auto unreachableVertices = std::vector<vertex_t*> {};
        std::set_difference( oldVertices.begin(), oldVertices.end()
                           , newVertices.begin(), newVertices.end()
                           , std::inserter(unreachableVertices, unreachableVertices.begin()) );

        // and release them
        for (auto v : unreachableVertices)
        {
            base::manager_.release(v);
        }     

        return this->reduce(diagram);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_manipulator<VertexData, ArcData, Allocator>::restrict_var
        (bdd_t&& diagram, const index_t i, const bool_t val) -> bdd_t
    {
        return bdd_t {std::move(this->restrict_var(diagram, i, val))};
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_manipulator<VertexData, ArcData, Allocator>::negate
        (bdd_t& diagram) -> bdd_t&
    {
        auto const trueLeaf  = diagram.true_leaf();
        auto const falseLeaf = diagram.false_leaf();

        if (trueLeaf)
        {
            auto& val = diagram.leafToVal_.at(trueLeaf);
            val = ! val;
        }

        if (falseLeaf)
        {
            auto& val = diagram.leafToVal_.at(falseLeaf);
            val = ! val;
        }

        return diagram;
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_manipulator<VertexData, ArcData, Allocator>::negate
        (bdd_t&& diagram) -> bdd_t
    {
        return bdd_t {std::move(this->negate(diagram))};
    }

    template<class VertexData, class ArcData, class Allocator>
    auto operator&& ( bdd<VertexData, ArcData, Allocator> const& lhs
                    , bdd<VertexData, ArcData, Allocator> const& rhs ) -> bdd<VertexData, ArcData, Allocator>
    {
        auto manipulator = bdd_manipulator<VertexData, ArcData, Allocator> {lhs.get_allocator()};
        return manipulator.apply(std::move(lhs), AND {}, std::move(rhs));
    }

    template<class VertexData, class ArcData, class Allocator>
    auto operator* ( bdd<VertexData, ArcData, Allocator> const& lhs
                   , bdd<VertexData, ArcData, Allocator> const& rhs ) -> bdd<VertexData, ArcData, Allocator>
    {
        auto manipulator = bdd_manipulator<VertexData, ArcData, Allocator> {lhs.get_allocator()};
        return manipulator.apply(std::move(lhs), AND {}, std::move(rhs));
    }
    
    template<class VertexData, class ArcData, class Allocator>
    auto operator|| ( bdd<VertexData, ArcData, Allocator> const& lhs
                    , bdd<VertexData, ArcData, Allocator> const& rhs ) -> bdd<VertexData, ArcData, Allocator>
    {
        auto manipulator = bdd_manipulator<VertexData, ArcData, Allocator> {lhs.get_allocator()};
        return manipulator.apply(std::move(lhs), OR {}, std::move(rhs));
    }
    
    template<class VertexData, class ArcData, class Allocator>
    auto operator+ ( bdd<VertexData, ArcData, Allocator> const& lhs
                   , bdd<VertexData, ArcData, Allocator> const& rhs ) -> bdd<VertexData, ArcData, Allocator>
    {
        auto manipulator = bdd_manipulator<VertexData, ArcData, Allocator> {lhs.get_allocator()};
        return manipulator.apply(std::move(lhs), OR {}, std::move(rhs));
    }
    
    template<class VertexData, class ArcData, class Allocator>
    auto operator^ ( bdd<VertexData, ArcData, Allocator> const& lhs
                   , bdd<VertexData, ArcData, Allocator> const& rhs ) -> bdd<VertexData, ArcData, Allocator>
    {
        auto manipulator = bdd_manipulator<VertexData, ArcData, Allocator> {lhs.get_allocator()};
        return manipulator.apply(std::move(lhs), XOR {}, std::move(rhs));
    }

    template<class VertexData, class ArcData, class Allocator>
    auto operator! ( bdd<VertexData, ArcData, Allocator>& lhs ) -> bdd<VertexData, ArcData, Allocator>&
    {
        auto manipulator = bdd_manipulator<VertexData, ArcData, Allocator> {lhs.get_allocator()};
        return manipulator.negate(lhs);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto operator! ( bdd<VertexData, ArcData, Allocator>&& lhs ) -> bdd<VertexData, ArcData, Allocator>
    {
        auto manipulator = bdd_manipulator<VertexData, ArcData, Allocator> {lhs.get_allocator()};
        return manipulator.negate(std::move(lhs));
    }
}

#endif