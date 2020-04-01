#ifndef _MIX_DD_BDD_
#define _MIX_DD_BDD_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <bitset>
#include <sstream>
#include <tuple>
#include <iterator>
#include <initializer_list>
#include "bool_f_input.hpp"
#include "../dd/graph.hpp"
#include "../dd/level_iterator.hpp"
#include "../dd/typedefs.hpp"
#include "../utils/io.hpp"
#include "../utils/string_utils.hpp"

namespace mix::dd
{   
    template<class VertexData, class ArcData> class bdd;
    template<class VertexData, class ArcData> class bdd_creator;
    template<class VertexData, class ArcData> class bdd_manipulator;

        template<class VertexData, class ArcData>
        class bdd_creator_alt;

        template<class VertexData, class ArcData>
        class bdd_tools;

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
        friend class bdd_manipulator<VertexData, ArcData>;
        friend class bdd_creator<VertexData, ArcData>;
            friend class bdd_creator_alt<VertexData, ArcData>;
            friend class bdd_tools<VertexData, ArcData>;
        friend auto swap<VertexData, ArcData> ( bdd<VertexData, ArcData>& lhs
                                              , bdd<VertexData, ArcData>& rhs ) noexcept -> void;

    public:
        using vertex_t     = vertex<VertexData, ArcData, 2>;
        using arc_t        = arc<VertexData, ArcData, 2>;
        using leaf_val_map = std::map<const vertex_t*, bool_t>;
        using labels_v     = std::vector<std::string>;

    public:
        using iterator       = dd_level_iterator<VertexData, ArcData, 2, vertex_t>;
        using const_iterator = dd_level_iterator<VertexData, ArcData, 2, const vertex_t>;

    private:
        vertex_t*    root_         {nullptr};
        index_t      variableCount {0};
        leaf_val_map leafToVal;
        labels_v     labels;

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
            @tparam BoolFunctionInput - type that stores values of the variables.
            @tparam GetVarVal - functor that returns value of i-th variable
                    from the input. std::vector<bool>, std::bitset<N>, bit_vector<N, bool_t>, 
                    var_vals_t are supported by default. 
                    See "bool_f_input.hpp" for implementation details. 
            @param  input values of the variables. 
            @return value of the function for given input. 
        */
        template< class BoolFunctionInput
                , class GetVarVal = get_var_val<BoolFunctionInput> >
        auto get_value (const BoolFunctionInput& input) const -> bool_t;

        /**
            TODO doc 
        */
        template< class BoolFunctionInput
                , class Container = std::vector<BoolFunctionInput>
                , class SetVarVal = set_var_val<BoolFunctionInput> >
        auto satisfy_all () const -> Container;

        /**
            TODO doc 
        */
        template< class BoolFunctionInput
                , class OutputIt
                , class SetVarVal = set_var_val<BoolFunctionInput> >
        auto satisfy_all (OutputIt out) const -> void;

        /**
            TODO doc 
        */
        auto truth_density () -> size_t;

        /**
            TODO doc 
            Doesn't use data field of the vertices but uses hash map internally,
            hence might be slower.
        */
        auto truth_density () const -> size_t;

        /**
            @return level order iterator. 
            // TODO doc
        */
        auto begin () -> iterator;
        
        auto end () -> iterator;
        
        auto begin () const -> const_iterator;
        
        auto end () const -> const_iterator;

        auto clone () const -> bdd;

        auto variable_count() const -> index_t;

        // TMP
        // auto leaf (const bool_t value) -> vertex_t*;
        auto true_leaf   () -> vertex_t*;
        auto false_leaf  () -> vertex_t*;
        auto get_root    () -> vertex_t*;

        auto set_labels (labels_v labels) -> void;
        auto set_labels (std::initializer_list<std::string> labels) -> void;

    private:
        bdd ( vertex_t* const pRoot
            , const index_t   pVariableCount
            , leaf_val_map    pLeafToVal );

        auto value            (const vertex_t* const v) const -> bool_t;
        auto is_leaf          (const vertex_t* const v) const -> bool;
        auto leaf_index       () const -> index_t;
        auto fill_levels      () const -> std::vector< std::vector<vertex_t*> >;
        auto indices          () const -> std::set<index_t>;
        auto calculate_alpha  (vertex_t* const v) -> size_t;

        auto label (const vertex_t* v) const -> std::string;

        template<class UnaryFunction>
        auto traverse ( vertex_t* const v
                      , UnaryFunction f ) const -> void;

        template<class Container>
        auto fill_container () const -> Container;

        template< class BoolFunctionInput
                , class InsertIterator
                , class SetVarVal = set_var_val<BoolFunctionInput> >
        auto satisfy_all_step ( const index_t         i
                              , const vertex_t* const v
                              , BoolFunctionInput&    xs
                              , InsertIterator&       out) const -> void;

        static auto are_equal ( vertex_t* const v1
                              , vertex_t* const v2
                              , const bdd&      d1
                              , const bdd&      d2 ) -> bool;
    };

    template<class VertexData, class ArcData>
    auto swap ( bdd<VertexData, ArcData>& lhs
              , bdd<VertexData, ArcData>& rhs ) noexcept -> void
    {
        using std::swap;
        
        swap(lhs.root_,          rhs.root_);
        swap(lhs.variableCount, rhs.variableCount);
        swap(lhs.leafToVal,     rhs.leafToVal);
    }

    template<class VertexData, class ArcData>
    bdd<VertexData, ArcData>::bdd(const bdd& other) :
        variableCount {other.variableCount}
    {
        if (! other.root_)
        {
            return;
        }

        // first we copy each vertex:
        std::map<const id_t, vertex_t*> newVerticesMap;
        other.traverse(other.root_, [&newVerticesMap](const vertex_t* const v) 
        {
            newVerticesMap.emplace(v->id, new vertex_t {*v});
        });

        // now we iterate the other diagram from the bottom level:
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
        this->root_ = newVerticesMap.at(levels.at(other.root_->index).front()->id);

        // fill leafToVal map:
        for (const auto [leaf, val] : other.leafToVal)
        {
            this->leafToVal.emplace(newVerticesMap.at(leaf->id), val);
        }
    }

    template<class VertexData, class ArcData>
    bdd<VertexData, ArcData>::bdd(bdd&& other) :
        root_          {other.root_}
      , variableCount {other.variableCount}  
      , leafToVal     {std::move(other.leafToVal)}
    {
        other.root_ = nullptr;
    }

    template<class VertexData, class ArcData>
    bdd<VertexData, ArcData>::bdd( vertex_t* const pRoot
                                 , const index_t   pVariableCount
                                 , leaf_val_map    pLeafToVal ) :
        root_          {pRoot}
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
        if (root_ == other.root_)
        {
            // Catches comparison with self and also case when both diagrams are empty.
            return true;
        }

        if (! root_ || ! other.root_)
        {
            // Case when one of the roots is null.
            return false;
        }

        if (variableCount != other.variableCount)
        {
            // Different number of variables. Can't be equal.
            return false;
        }

        // Compare trees.
        return bdd::are_equal(root_, other.root_, *this, other);
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
        std::ostringstream finalGraphOstr;

        std::vector< std::vector<const vertex_t*> > levelGroups(this->variableCount + 1);
        std::vector< std::tuple<id_t, id_t, bool> > arcs;

        this->traverse(this->root_, [&](const vertex_t* const v) {
            if (! v->is_leaf())
            {
                arcs.push_back(std::make_tuple(v->id, v->son(0)->id, false));
                arcs.push_back(std::make_tuple(v->id, v->son(1)->id, true));                                              
            }

            levelGroups[v->index].push_back(v);
        });

        finalGraphOstr << "digraph D {" << utils::EOL;

        // node shape
        finalGraphOstr << "    node [shape = square] ";
        for (auto& [key, val] : this->leafToVal)
        {
            finalGraphOstr << key->id << ' ';
        }
        finalGraphOstr << ";"                          << utils::EOL
                       << "    node [shape = circle];" << utils::EOL << utils::EOL;

        // labels
        for (size_t level {0}; level < levelGroups.size(); ++level)
        {
            for (auto v : levelGroups[level])
            {
                finalGraphOstr << "    " << v->id << " [label = " << label(v) << "];" << utils::EOL;
            }
        }
        finalGraphOstr << utils::EOL;
    
        // arcs
        for (auto& arc : arcs)
        {
            const auto from  {std::get<0>(arc)};
            const auto to    {std::get<1>(arc)};
            const auto style {std::get<2>(arc) ? "solid" : "dashed"};

            finalGraphOstr << "    " << from << " -> " << to << " [style = " << style << "];" << utils::EOL;
        }

        finalGraphOstr << utils::EOL;

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

        this->traverse(this->root_, [&size](const vertex_t* const)
        {
            ++size;
        });

        return size;
    }

    template<class VertexData, class ArcData>
    template<class BoolFunctionInput, class GetVarVal>
    auto bdd<VertexData, ArcData>::get_value 
        (const BoolFunctionInput& input) const -> bool_t
    {
        GetVarVal get_val;

        auto v = root_;

        while (! this->is_leaf(v))
        {
            v = v->son(get_val(input, v->index));
        }
        
        return this->leafToVal.at(v);
    }

    template<class VertexData, class ArcData>
    template<class BoolFunctionInput, class Container, class SetVarVal>
    auto bdd<VertexData, ArcData>::satisfy_all
        () const -> Container
    {
        auto c   = Container {};
        auto xs  = BoolFunctionInput {};
        auto out = std::inserter(c, std::end(c));
        
        satisfy_all_step(0, root_, xs, out);
        
        return c;
    }

    template<class VertexData, class ArcData>
    template<class BoolFunctionInput, class OutputIt, class SetVarVal>
    auto bdd<VertexData, ArcData>::satisfy_all
        (OutputIt out) const -> void
    {
        auto xs {BoolFunctionInput {}};
        satisfy_all_step(0, root_, xs, out);
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::truth_density
        () -> size_t
    {
        // TODO maybe add another numberic types
        if constexpr (! std::is_same_v<double, VertexData>)
        {
            return const_cast<const bdd&>(*this).truth_density();
        }
        else
        {
            this->calculate_alpha(root_);
            // return root_->data * utils::two_pow(0 == root_->index ? 0 : root_->index - 1);
            return root_->data * utils::two_pow(root_->index);
        }
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::truth_density
        () const -> size_t
    {
        throw "Not supported yet!";
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::begin
        () -> iterator
    {
        return iterator {this->root_, this->variableCount};
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::end
        () -> iterator
    {
        return iterator {nullptr, this->variableCount};
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::clone
        () const -> bdd
    {
        return bdd {*this};
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::variable_count
        () const -> index_t
    {
        return variableCount;
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

        if (this->root_)
        {
            this->traverse(this->root_, [this, &levels](vertex_t* const v) 
            {
                levels[v->index].push_back(v);
            });
        }

        return levels;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::indices
        () const -> std::set<index_t>
    {
        std::set<index_t> indices;

        this->traverse(this->root_, [this, &indices](const vertex_t* const v)
        {
            if (! this->is_leaf(v))
            {
                indices.insert(v->index);
            }
        });

        return indices;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::calculate_alpha
        (vertex_t* const v) -> size_t
    {
        using utils::two_pow;

        v->mark = ! v->mark;

        if (! this->is_leaf(v) && v->mark != v->son(0)->mark)
        {
            this->calculate_alpha(v->son(0));
        }

        if (! this->is_leaf(v) && v->mark != v->son(1)->mark)
        {
            this->calculate_alpha(v->son(1));
        }

        v->data = this->is_leaf(v) ? this->value(v)
                                   : v->son(0)->data * two_pow(v->son(0)->index - v->index - 1)
                                   + v->son(1)->data * two_pow(v->son(1)->index - v->index - 1);
        
        return v->data;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::label
        (const vertex_t* v) const -> std::string
    {
        using std::to_string;
        const auto i {v->index};
        return i < this->labels.size() ? this->labels.at(i) : 
               i == this->leaf_index() ? to_string(this->leafToVal.at(v)) :
                                         "x" + std::to_string(i);
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::true_leaf
        () -> vertex_t*
    {
        for (auto [key, val] : this->leafToVal)
        {
            if (1 == val)
            {
                return const_cast<vertex_t*>(key);
            }
        }

        return nullptr;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::false_leaf
        () -> vertex_t*
    {
        for (auto [key, val] : this->leafToVal)
        {
            if (0 == val)
            {
                return const_cast<vertex_t*>(key);
            }
        }

        return nullptr;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::get_root
        () -> vertex_t*
    {
        return this->root_;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::set_labels
        (labels_v ls) -> void
    {
        this->labels = std::move(ls);
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::set_labels
        (std::initializer_list<std::string> ls) -> void
    {
        this->labels = ls;
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
    template<class Container>
    auto bdd<VertexData, ArcData>::fill_container
        () const -> Container
    {
        Container c;

        auto outIt {std::inserter(c, std::end(c))};

        this->traverse(this->root_, [&c, &outIt](vertex_t* const v)
        {
            outIt = v;
        });

        return c;
    }

    template<class VertexData, class ArcData>
    template<class BoolFunctionInput, class InsertIterator, class SetVarVal>
    auto bdd<VertexData, ArcData>::satisfy_all_step 
        ( const index_t         i
        , const vertex_t* const v
        , BoolFunctionInput&    xs
        , InsertIterator&       out ) const -> void
    {
        SetVarVal set_var;

        if (0 == this->value(v))
        {
            return;
        }
        else if (i == this->leaf_index() && 1 == this->value(v))
        {
            out = xs;
            return;
        }
        else if (v->index > i)
        {
            set_var(xs, i, 0);
            satisfy_all_step(i + 1, v, xs, out);
            set_var(xs, i, 1);
            satisfy_all_step(i + 1, v, xs, out);
        }
        else
        {
            set_var(xs, i, 0);
            satisfy_all_step(i + 1, v->son(0), xs, out);
            set_var(xs, i, 1);
            satisfy_all_step(i + 1, v->son(1), xs, out);
        }
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::are_equal 
        ( vertex_t* const v1
        , vertex_t* const v2
        , const bdd&      d1
        , const bdd&      d2 ) -> bool
    {
        if (v1->index != v2->index)
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

        auto const equalOnTheLeft = are_equal(v1->son(0), v2->son(0), d1, d2);
        if (! equalOnTheLeft)
        {
            // Left subtrees aren't equal => can't be equal.
            return false;
        }

        auto equalOnTheRight = are_equal(v1->son(1), v2->son(1), d1, d2);
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