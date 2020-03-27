#ifndef _MIX_DD_BDD_TOOLS_
#define _MIX_DD_BDD_TOOLS_

#include <set>
#include <vector>
#include <algorithm>
#include <iterator>
#include <ostream>
#include "bdd.hpp"
#include "bdd_reducer.hpp"
#include "bdd_pla.hpp"
#include "bdd_manipulator.hpp"

namespace mix::dd
{
    /**
        Contains static method for creating and manipulating Ordered Binary Decision Diagrams.
    */
    template<class VertexData, class ArcData>
    struct bdd_tools
    {
        using bdd_t    = bdd<VertexData, ArcData>;
        using vertex_t = typename bdd<VertexData, ArcData>::vertex_t;

        /**
            Reads .pla file from given path and creates diagrams for
            functions defined in that file.
            @param  filePath - path to the file.
            @return vector of diagrams.
        */
        static auto create_from_pla (const std::string& filePath) -> std::vector<bdd_t>;

        /**
            Reads .pla file from given path and creates only one diagram.
            If there are multiple functions defined in the file only first
            one is considered.
            @param  filePath - path to the file.
            @return diagram.
        */
        static auto create_one_from_pla (const std::string& filePath) -> bdd_t;

        /**
            @return diagram with single leaf with value true.
        */
        static auto create_true () -> bdd_t;

        /**
            @return diagram with single leaf with value false.
        */
        static auto create_false () -> bdd_t;

        /**
            @param i - index of the variable.
            @return diagram representing variable with given index.
        */
        static auto create_var (const index_t i) -> bdd_t;

        /**
            Creates new diagram as a result of merging @p d1 and @p d2
            using @p op . Supported operators are:
            AND, OR, NAND, NOR, XOR.
            @param  d1 -> first diagram.
            @param  d2 -> second diagram.
            @param  op -> one of AND{}, OR{}, NAND{}, NOR{}, XOR{}.
            @return new diagram.
        */
        template<class BinaryBoolOperator>
        static auto merge ( const bdd_t& d1
                          , const bdd_t& d2
                          , BinaryBoolOperator op ) -> bdd_t;

        /**
            Appliest logical NOT on @p diagram .
            @param diagram - diagram to be nagted.
            @return referecen to @p diagram .
        */
        static auto negate (bdd_t& diagram) -> bdd_t&;

        /**
            Performs restriction by i-th variable having given value.
            @param diagram - diagram to be restricted.
            @param i       - index of the variable.
            @param val     - value of the variable.
            @return reference to diagram.
        */
        static auto restrict_by ( bdd_t& diagram
                                , const index_t i
                                , const bool_t val ) -> bdd_t&;

        /**
            @param f diagram representing boolean function.
            @param i index of the variable to derivate by.
            @return  new diagram that represents derivative 
                     of @p f by @p i - th variable.
        */
        static auto derivative ( const bdd_t& f 
                               , const index_t i ) -> bdd_t;
    };

    /**
        Prints representation of @p diagram into @p ost .
        @param ost     - output stream.
        @param diagram - diagram to be printed.
        @return reference to @p ost .
    */
    template<class VertexData, class ArcData>
    auto operator<< ( std::ostream& ost
                    , const bdd<VertexData, ArcData>& diagram ) -> std::ostream&;

    /**
        @param i - index of the variable.
        @return diagram representing variable with index @p i .
    */
    template<class VertexData = double, class ArcData = empty_t>
    auto x (const index_t i) -> bdd<VertexData, ArcData>;

    /**
        Merges diagrams using AND.
    */
    template<class VertexData, class ArcData>
    auto operator&& ( const bdd<VertexData, ArcData>& lhs
                    , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>;
    
    /**
        Merges diagrams using AND.
    */
    template<class VertexData, class ArcData>
    auto operator* ( const bdd<VertexData, ArcData>& lhs
                   , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>;

    /**
        Merges diagrams using OR.
    */
    template<class VertexData, class ArcData>
    auto operator|| ( const bdd<VertexData, ArcData>& lhs
                    , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>;

    /**
        Merges diagrams using OR.
    */
    template<class VertexData, class ArcData>
    auto operator+ ( const bdd<VertexData, ArcData>& lhs
                   , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>;
    
    /**
        Merges diagrams using XOR.
    */
    template<class VertexData, class ArcData>
    auto operator^ ( const bdd<VertexData, ArcData>& lhs
                   , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>;

    /**
        Merges diagrams using NAND.
    */
    template<class VertexData, class ArcData>
    auto nand ( const bdd<VertexData, ArcData>& lhs
              , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>;

    /**
        Merges diagrams using NOR.
    */
    template<class VertexData, class ArcData>
    auto nor ( const bdd<VertexData, ArcData>& lhs
             , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>;

    /**
        Creates new diagram as negation of @p lhs .
    */
    template<class VertexData, class ArcData>
    auto operator! (const bdd<VertexData, ArcData>& lhs) -> bdd<VertexData, ArcData>;

// implementation follows:
    
    template<class VertexData, class ArcData>
    auto operator<< ( std::ostream& ost
                    , const bdd<VertexData, ArcData>& diagram ) -> std::ostream&
    {
        ost << diagram.to_dot_graph() << std::endl;
        return ost;
    }

    template<class VertexData, class ArcData>
    auto x (const index_t i) -> bdd<VertexData, ArcData>
    {
        return bdd_tools<VertexData, ArcData>::create_var(i);
    }

    template<class VertexData, class ArcData>
    auto bdd_tools<VertexData, ArcData>::create_from_pla
        (const std::string& filePath) -> std::vector<bdd_t>
    {
        bdd_creator<VertexData, ArcData> plaCreator;
        return plaCreator.create_i(pla_file::load_from_file(filePath));
    }

    template<class VertexData, class ArcData>
    auto bdd_tools<VertexData, ArcData>::create_one_from_pla
        (const std::string& filePath) -> bdd_t
    {
        return create_from_pla(filePath).front();
    }

    template<class VertexData, class ArcData>
    auto bdd_tools<VertexData, ArcData>::create_true
        () -> bdd_t
    {
        return bdd_creator_alt<VertexData, ArcData>::just_true();
    }

    template<class VertexData, class ArcData>
    auto bdd_tools<VertexData, ArcData>::create_false
        () -> bdd_t
    {
        return bdd_creator_alt<VertexData, ArcData>::just_false();
    }

    template<class VertexData, class ArcData>
    auto bdd_tools<VertexData, ArcData>::create_var
        (const index_t i) -> bdd_t
    {
        return bdd_creator_alt<VertexData, ArcData>::just_var(i);
    }

    template<class VertexData, class ArcData>
    template<class BinaryBoolOperator>
    auto bdd_tools<VertexData, ArcData>::merge
        (const bdd_t& d1, const bdd_t& d2, BinaryBoolOperator op) -> bdd_t
    {
        bdd_manipulator<VertexData, ArcData> merger;
        return merger.merge(d1, d2, op);
    }

    template<class VertexData, class ArcData>
    auto bdd_tools<VertexData, ArcData>::negate
        (bdd_t& diagram) -> bdd_t&
    {
        const vertex_t* trueLeaf  {nullptr};
        const vertex_t* falseLeaf {nullptr};

        for (const auto& [leaf, val] : diagram.leafToVal)
        {
            if (0 == val)
            {
                falseLeaf = leaf;
            }

            if (1 == val)
            {
                trueLeaf = leaf;
            }
        }

        diagram.leafToVal.clear();
        
        if (trueLeaf)
        {
            diagram.leafToVal.emplace(trueLeaf, 0);
        }

        if (falseLeaf)
        {
            diagram.leafToVal.emplace(falseLeaf, 1);
        }

        return diagram;
    }

    template<class VertexData, class ArcData>
    auto bdd_tools<VertexData, ArcData>::restrict_by
        (bdd_t& diagram, const index_t i, const bool_t val) -> bdd_t&
    {
        if (i >= diagram.variableCount)
        {
            return diagram;
        }

        const auto oldVertices 
        {
            diagram.template fill_container< std::set<vertex_t*> >()
        };

        // "skip" all vertices with given index
        diagram.traverse(diagram.root_, [i, val](vertex_t* const v)
        {
            if (v->is_leaf())
            {
                return;
            }

            if (! v->son(0)->is_leaf() && i == v->son(0)->index)
            {
                v->son(0) = v->son(0)->son(val);
            }

            if (! v->son(1)->is_leaf() && i == v->son(1)->index)
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
        const auto newVertices 
        {
            diagram.template fill_container< std::set<vertex_t*> >()
        };
        std::vector<vertex_t*> unreachableVertices;
        std::set_difference( oldVertices.begin(), oldVertices.end()
                           , newVertices.begin(), newVertices.end()
                           , std::inserter(unreachableVertices, unreachableVertices.begin()) );

        // and delete them
        for (auto v : unreachableVertices)
        {
            delete v;
        }     

        bdd_reducer<VertexData, ArcData> reducer;
        reducer.reduce(diagram);

        return diagram;
    }

    template<class VertexData, class ArcData>
    auto bdd_tools<VertexData, ArcData>::derivative
        (const bdd_t& f, const index_t i) -> bdd_t
    {
        return restrict_by(bdd_t {f}, i, 0) ^ restrict_by(bdd_t {f}, i, 1);
    }

    template<class VertexData, class ArcData>
    auto operator&& ( const bdd<VertexData, ArcData>& lhs
                    , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> merger;
        return merger.merge(lhs, rhs, AND {});
    }

    template<class VertexData, class ArcData>
    auto operator* ( const bdd<VertexData, ArcData>& lhs
                   , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> merger;
        return merger.merge(lhs, rhs, AND {});
    }
    
    template<class VertexData, class ArcData>
    auto operator|| ( const bdd<VertexData, ArcData>& lhs
                    , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> merger;
        return merger.merge(lhs, rhs, OR {});
    }
    
    template<class VertexData, class ArcData>
    auto operator+ ( const bdd<VertexData, ArcData>& lhs
                   , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> merger;
        return merger.merge(lhs, rhs, OR {});
    }
    
    template<class VertexData, class ArcData>
    auto operator^ ( const bdd<VertexData, ArcData>& lhs
                   , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> merger;
        return merger.merge(lhs, rhs, XOR {});
    }

    template<class VertexData, class ArcData>
    auto nand ( const bdd<VertexData, ArcData>& lhs
              , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> merger;
        return merger.merge(lhs, rhs, NAND {});
    }

    template<class VertexData, class ArcData>
    auto nor ( const bdd<VertexData, ArcData>& lhs
             , const bdd<VertexData, ArcData>& rhs ) -> bdd<VertexData, ArcData>
    {
        bdd_manipulator<VertexData, ArcData> merger;
        return merger.merge(lhs, rhs, NOR {});
    }

    template<class VertexData, class ArcData>
    auto operator! (const bdd<VertexData, ArcData>& lhs) -> bdd<VertexData, ArcData>
    {
        bdd<VertexData, ArcData> copy {lhs};
        bdd_tools<VertexData, ArcData>::negate(copy);
        return copy;
    }
}

#endif