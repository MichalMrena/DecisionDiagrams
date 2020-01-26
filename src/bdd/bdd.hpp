#ifndef _MIX_DD_BDD_
#define _MIX_DD_BDD_

#include <string>
#include <vector>
#include <map>
#include <bitset>
#include <sstream>
#include <tuple>
#include <iterator>
#include "bool_f_input.hpp"
#include "../dd/graph.hpp"
#include "../dd/typedefs.hpp"
#include "../utils/io.hpp"
#include "../utils/string_utils.hpp"

namespace mix::dd
{   
    template<class VertexData, class ArcData>
    class bdd;
    
    template<class VertexData, class ArcData>
    class bdd_creator;

    template<class VertexData, class ArcData>
    class bdd_merger;

    template<class VertexData, class ArcData>
    class bdd_reducer;

    template<class VertexData, class ArcData>
    class bdds_from_pla;

    template<class VertexData, class ArcData>
    auto swap ( bdd<VertexData, ArcData>& lhs
              , bdd<VertexData, ArcData>& rhs ) noexcept -> void;

    /**
        Ordered Binary Decision Diagram.

        @tparam VertexData - type of the data that will be stored in vertices of the diagram.
                Use empty_t defined in "./src/dd/graph.hpp" if you don't need to store any data.
        @tparam ArcData - type of the data that will be stored in arcs of the diagram.
                Use empty_t defined in "./src/dd/graph.hpp" if you don't need to store any data.
    */
    template<class VertexData, class ArcData>
    class bdd
    {
    public:
        friend class bdd_creator<VertexData, ArcData>;
        friend class bdd_merger<VertexData, ArcData>;
        friend class bdd_reducer<VertexData, ArcData>;
        friend class bdds_from_pla<VertexData, ArcData>;
        friend auto swap<VertexData, ArcData> ( bdd<VertexData, ArcData>& lhs
                                              , bdd<VertexData, ArcData>& rhs ) noexcept -> void;

    private:
        using vertex_t     = vertex<VertexData, ArcData, 2>;
        using arc_t        = arc<VertexData, ArcData, 2>;
        using leaf_val_map = std::map<const vertex_t*, bool_t>;

    private:
        vertex_t*    root          {nullptr};
        index_t      variableCount {0};
        leaf_val_map leafToVal;

    public:
        /**
            @return diagram with single leaf that has value true.
        */
        static auto just_true () -> bdd;

        /**
            @return diagram with single leaf that has value false.
        */
        static auto just_false () -> bdd;

        /**
            @param  index - index of the variable.
            @return diagram representing single variable.
        */
        static auto just_var (const index_t index) -> bdd;

    public:
        /**
            Default constructed diagram is empty.
            Don't use diagram created this way.
        */
        bdd  () = default;

        /**
            Copy constructor.
        */
        bdd  (const bdd& other);

        /**
            Move constructor.
        */
        bdd  (bdd&& other);

        /**
            Desctructor.
        */
        ~bdd ();

        /**
            Copy and move assign operator.
            See. https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
           
            @param rhs - diagram that is to be asigned into this one.
        */
        auto operator= (bdd rhs) -> bdd&;
        
        /**
            @param  rhs   - other diagram that should be compared with this.
            @return true  if this and rhs diagrams represent the same function.
                    false otherwise
        */
        auto operator== (const bdd& rhs) const -> bool;

        /**
            @param  rhs   - other diagram that should be compared with this.
            @return false if this and rhs diagrams represents the same function.
                    true  otherwise.
        */
        auto operator!= (const bdd& rhs) const -> bool;

        /**
            Picture can be viewed here http://www.webgraphviz.com/.
           
            @return string with dot representation of this diagram.
        */
        auto to_dot_graph () const -> std::string;

        /**
            @return Number of vertices in the diagram.
        */
        auto vertex_count () const -> size_t;
        
        /**
            Performs logical not over this diagram.
            
            @return reference to this diagram.
        */
        auto negate () -> bdd&;

        /**
            @tparam BoolFunctionInput - values of variables.
            @tparam GetVarVal - functor that returns value of i-th variable
                    from the input. std::vector<bool>, std::bitset<N>, bit_vector<N, bool_t>, 
                    var_vals_t are supported by default. 
                    See "bool_f_input.hpp" for implementation details.  
            @return value of the function for given input. 
        */
        template< class BoolFunctionInput
                , class GetVarVal = get_var_val<BoolFunctionInput> >
        auto get_value (const BoolFunctionInput& input) const -> bool_t;

    private:
        bdd ( vertex_t* const pRoot
            , const index_t   pVariableCount
            , leaf_val_map    pLeafToVal );

        auto value       (const vertex_t* const v) const -> bool_t;
        auto is_leaf     (const vertex_t* const v) const -> bool;
        auto leaf_index  () const -> index_t;
        auto fill_levels () const -> std::vector< std::vector<vertex_t*> >;

        template<class UnaryFunction>
        auto traverse ( vertex_t* const v
                      , UnaryFunction f ) const -> void;

        static auto are_equal ( vertex_t* const v1
                              , vertex_t* const v2
                              , const bdd& d1
                              , const bdd& d2 ) -> bool;
    };

    template<class VertexData, class ArcData>
    auto swap ( bdd<VertexData, ArcData>& lhs
              , bdd<VertexData, ArcData>& rhs ) noexcept -> void
    {
        using std::swap;
        
        swap(lhs.root,          rhs.root);
        swap(lhs.variableCount, rhs.variableCount);
        swap(lhs.leafToVal,     rhs.leafToVal);
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::just_true
        () -> bdd
    {
        vertex_t* const trueLeaf {new vertex_t {1, 0}};
        
        leaf_val_map leafValMap
        {
            {trueLeaf, 1}
        };
        
        return bdd {trueLeaf, 0, std::move(leafValMap)};
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::just_false
        () -> bdd
    {
        vertex_t* const falseLeaf {new vertex_t {1, 0}};
       
        leaf_val_map leafValMap
        {
            {falseLeaf, 0}
        };

        return bdd {falseLeaf, 0, std::move(leafValMap)};
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::just_var
        (const index_t index) -> bdd
    {
        vertex_t* const falseLeaf {new vertex_t {1, index + 1}};
        vertex_t* const trueLeaf  {new vertex_t {2, index + 1}};
        vertex_t* const varVertex {new vertex_t {3, index, {arc_t {falseLeaf}, arc_t {trueLeaf}}}};
        
        leaf_val_map leafValMap
        {
            {falseLeaf, 0}
          , {trueLeaf, 1}
        };

        return bdd {varVertex, index + 1, std::move(leafValMap)};
    }

    template<class VertexData, class ArcData>
    bdd<VertexData, ArcData>::bdd(const bdd& other) :
        variableCount {other.variableCount}
    {
        if (! other.root)
        {
            return;
        }

        // first we copy each vertex:
        std::map<const id_t, vertex_t*> newVerticesMap;
        other.traverse(other.root, [&newVerticesMap](const vertex_t* const v) 
        {
            newVerticesMap.emplace(v->id, new vertex_t {*v});
        });

        // now we iterate other diagram from the bottom level:
        const auto levels {other.fill_levels()};
        auto levelsIt     {levels.rbegin()};
        auto levelsItEnd  {levels.rend()};
        ++levelsIt;

        // making deep copy here:
        while (levelsIt != levelsItEnd)
        {
            for (const auto otherVertex : *levelsIt)
            {
                auto newVertex {newVerticesMap.at(otherVertex->id)};
                newVertex->son(0) = newVerticesMap.at(otherVertex->son(0)->id);
                newVertex->son(1) = newVerticesMap.at(otherVertex->son(1)->id);
            }

            ++levelsIt;
        }

        // set new root:
        this->root = newVerticesMap.at(levels.at(other.root->index).front()->id);

        // fill leafToVal map:
        for (const auto [leaf, val] : other.leafToVal)
        {
            this->leafToVal.emplace(newVerticesMap.at(leaf->id), val);
        }
    }

    template<class VertexData, class ArcData>
    bdd<VertexData, ArcData>::bdd(bdd&& other) :
        root          {other.root}
      , variableCount {other.variableCount}  
      , leafToVal     {std::move(other.leafToVal)}
    {
        other.root = nullptr;
    }

    template<class VertexData, class ArcData>
    bdd<VertexData, ArcData>::bdd(vertex_t* const pRoot
                                , const index_t   pVariableCount
                                , leaf_val_map    pLeafToVal) :
        root          {pRoot}
      , variableCount {pVariableCount}  
      , leafToVal     {std::move(pLeafToVal)}
    {
    }

    template<class VertexData, class ArcData>
    bdd<VertexData, ArcData>::~bdd()
    {
        for (const auto& level : this->fill_levels())
        {
            for (const auto v : level)
            {
                delete v;
            }
        }
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::operator= (bdd rhs) -> bdd&
    {
        using std::swap;
        swap(*this, rhs);
        return *this;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::operator==
        (const bdd& other) const -> bool 
    {
        if (this->root == other.root)
        {
            // Catches comparison with self and also case when both diagrams are empty.
            return true;
        }

        if (! this->root || ! other.root)
        {
            // Case when one of the roots is null.
            return false;
        }

        const auto areEqual {are_equal(this->root, other.root, *this, other)};

        if (! areEqual)
        {
            // Traverse both trees in order to correctly set marks.
            // Since they are not equal only a part of each tree was traversed.
            this->traverse(this->root, [](auto) {});
            this->traverse(other.root, [](auto) {});
        }

        return areEqual;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::operator!=
        (const bdd& other) const -> bool 
    {
        return ! (*this == other);
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::to_dot_graph 
        () const -> std::string
    {
        using std::to_string;

        std::ostringstream finalGraphOstr;

        std::vector< std::vector<const vertex_t*> > levelGroups(this->variableCount + 1);
        std::vector< std::tuple<id_t, id_t, bool> > arcs;

        this->traverse(this->root, [&](const vertex_t* const v) {
            if (! v->is_leaf())
            {
                arcs.push_back(std::make_tuple( v->id
                                              , v->son(0)->id
                                              , false));

                arcs.push_back(std::make_tuple( v->id
                                              , v->son(1)->id
                                              , true));                                              
            }

            levelGroups[v->index].push_back(v);
        });

        finalGraphOstr << "digraph D {" << utils::EOL;

        // node shape
        finalGraphOstr << "    "
                       << "node [shape = square] ";
        for (auto& [key, val] : this->leafToVal)
        {
            finalGraphOstr << key->id << ' ';
        }
        finalGraphOstr << ";"                      << utils::EOL
                       << "    " 
                       << "node [shape = circle];" << utils::EOL << utils::EOL;

        // labels
        for (size_t level {0}; level < levelGroups.size(); ++level)
        {
            for (auto v : levelGroups[level])
            {
                const std::string label 
                {
                    level != this->leaf_index() 
                        ? ("x" + to_string(level))
                        : to_string(this->leafToVal.at(v))
                };
                
                finalGraphOstr << "    "
                               << v->id 
                               << " [label = " << label << "];" 
                               << utils::EOL;
            }
        }
        finalGraphOstr << utils::EOL;
    
        // arcs
        for (auto& arc : arcs)
        {
            const auto from  {std::get<0>(arc)};
            const auto to    {std::get<1>(arc)};
            const auto style {std::get<2>(arc) ? "solid" : "dashed"};

            finalGraphOstr << "    "
                           << to_string(from) 
                           << " -> "
                           << to_string(to)
                           << " [style = " << style << "];"
                           << utils::EOL;
        }

        // same rank
        for (size_t level {0}; level < levelGroups.size(); ++level)
        {
            if (levelGroups[level].empty())
            {
                continue;
            }
            
            finalGraphOstr << "    {rank = same; ";

            for (auto v : levelGroups[level])
            {
                finalGraphOstr << v->id << "; ";
            }

            finalGraphOstr << "}" << utils::EOL;
        }

        finalGraphOstr << '}' << utils::EOL;

        return finalGraphOstr.str();
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::vertex_count
        () const -> size_t
    {
        size_t size {0};

        this->traverse(this->root, [&size](const vertex_t* const)
        {
            ++size;
        });

        return size;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::negate
        () -> bdd&
    {
        const vertex_t* trueLeaf  {nullptr};
        const vertex_t* falseLeaf {nullptr};

        for (const auto& [leaf, val] : this->leafToVal)
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

        this->leafToVal.clear();
        
        if (trueLeaf)
        {
            this->leafToVal.emplace(trueLeaf, 0);
        }

        if (falseLeaf)
        {
            this->leafToVal.emplace(falseLeaf, 1);
        }

        return *this;
    }

    template<class VertexData, class ArcData>
    template<class BoolFunctionInput, class GetVarVal>
    auto bdd<VertexData, ArcData>::get_value 
        (const BoolFunctionInput& input) const -> bool_t
    {
        GetVarVal get_val;

        auto v {this->root};

        while (! this->is_leaf(v))
        {
            v = v->son(get_val(input, v->index));
        }
        
        return this->leafToVal.at(v);
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::value 
        (const vertex_t* const v) const -> bool_t
    {
        return this->is_leaf(v) ? this->leafToVal.at(v) 
                                : X;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::is_leaf
        (const vertex_t* const v) const -> bool
    {
        return v->index == this->leaf_index();
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::leaf_index
        () const -> index_t
    {
        return this->variableCount;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::fill_levels
        () const -> std::vector< std::vector<vertex_t*> >
    {
        std::vector< std::vector<vertex_t*> > levels(this->variableCount + 1);

        if (this->root)
        {
            this->traverse(this->root, [this, &levels](vertex_t* const v) 
            {
                levels[v->index].push_back(v);
            });
        }

        return levels;
    }

    template<class VertexData, class ArcData>
    template<class UnaryFunction>
    auto bdd<VertexData, ArcData>::traverse 
        (vertex_t* const v, UnaryFunction f) const -> void
    {
        v->mark = ! v->mark;

        f(v);

        if (this->is_leaf(v))
        {
            return;
        }

        if (v->mark != v->son(0)->mark)
        {
            this->traverse(v->son(0), f);
        }

        if (v->mark != v->son(1)->mark)
        {
            this->traverse(v->son(1), f);
        }
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::are_equal 
        ( vertex_t* const v1
        , vertex_t* const v2
        , const bdd& d1
        , const bdd& d2 ) -> bool
    {
        if (v1->index != v2->index)
        {
            // Different indices, can't be equal.
            return false;
        }

        if (d1.is_leaf(v1) != d2.is_leaf(v2))
        {
            // One of them is not leaf.
            return false;
        }

        v1->mark = ! v1->mark;
        v2->mark = ! v2->mark;

        if (d1.is_leaf(v1) && d2.is_leaf(v2))
        {
            // Both are leafs so their values should match.
            return d1.leafToVal.at(v1) == d2.leafToVal.at(v2);
        }

        const auto canV1DescendLeft  {v1->mark != v1->son(0)->mark};
        const auto canV2DescendLeft  {v2->mark != v2->son(0)->mark};

        if (canV1DescendLeft != canV2DescendLeft)
        {
            // Different state on the left so they can't be equal.
            return false;
        }

        const auto equalOnTheLeft 
        {
            // Possibly search the left subtree.
            canV1DescendLeft ? are_equal(v1->son(0), v2->son(0), d1, d2) : true
        };

        if (! equalOnTheLeft)
        {
            return false;
        }

        const auto canV1DescendRight {v1->mark != v1->son(1)->mark};
        const auto canV2DescendRight {v2->mark != v2->son(1)->mark};

        if (canV1DescendRight != canV2DescendRight)
        {
            // Different state on the right so they can't be equal.
            return false;
        }

        const auto equalOnTheRight 
        {
            // Possibly search the right subtree.
            canV1DescendRight ? are_equal(v1->son(1), v2->son(1), d1, d2) : true
        };

        if (! equalOnTheRight)
        {
            return false;
        }

        // Can't descend to either. Decision will be made somewhere else.
        return true;
    }


    template<class VertexData = empty_t, class ArcData = empty_t>
    auto x (const index_t index) -> bdd<VertexData, ArcData>
    {
        return bdd<VertexData, ArcData>::just_var(index);
    }
}

#endif