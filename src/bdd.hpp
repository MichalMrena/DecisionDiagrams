#ifndef _MIX_DD_BDD_
#define _MIX_DD_BDD_

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <bitset>
#include <sstream>
#include <tuple>
#include "graph.hpp"
#include "typedefs.hpp"
#include "utils/string_utils.hpp"
#include "data_structures/list_map.hpp"

namespace mix::dd
{   
    template<class VertexData, class ArcData>
    class bdd_creator;

    template<class VertexData, class ArcData>
    class bdd_merger;

    template<class VertexData, class ArcData>
    class bdd_reducer;

    template<class VertexData, class ArcData>
    class bdds_from_pla;

    template<class VertexData, class ArcData>
    class bdd
    {
    private:
        using vertex_t     = vertex<VertexData, ArcData, 2>;
        using arc_t        = arc<VertexData, ArcData, 2>;
        using leaf_val_map = list_map<const vertex_t*, log_val_t>;

    private:    
        vertex_t* root          {nullptr};
        index_t   variableCount {0};
        std::map<const vertex_t*, log_val_t> leafToVal;

    public:
        friend class bdd_creator<VertexData, ArcData>;
        friend class bdd_merger<VertexData, ArcData>;
        friend class bdd_reducer<VertexData, ArcData>;
        friend class bdds_from_pla<VertexData, ArcData>;

    public:
        static auto TRUE     () -> bdd;
        static auto FALSE    () -> bdd;
        static auto VARIABLE (const size_t index) -> bdd;

    public:
        bdd() = default;
        bdd(const bdd& other) = delete; // not yet implemented
        bdd(bdd&& other);
        ~bdd();

        // TODO use vertex allocator insted of new

        // TODO copy swap
        auto operator= (bdd&& rhs) -> bdd&;

        auto to_dot_graph () const -> std::string;
        
        // TODO na vstupe asi bitset keby mala funkcia > ako 64 premenných
        auto get_value (const input_t input) const -> log_val_t;

        auto get_value (const std::vector<bool>& varVals) const -> log_val_t;
        
        template<size_t N>
        auto get_value (const std::bitset<N>& varVals) const -> log_val_t;

    public: // just tmp private later
        bdd(vertex_t* const pRoot
          , const index_t   pVariableCount
          , std::map<const vertex_t*, log_val_t>&& pLeafToVal);
          // TODO nie rval ref ale value, ktora sa vytvori move construtom

        auto value   (const vertex_t* const v) const -> log_val_t;
        auto is_leaf (const vertex_t* const v) const -> bool;

        auto leaf_index () const -> index_t;

        template<class UnaryFunction>
        auto traverse (vertex_t* const v, UnaryFunction f) const -> void;

        static auto low  (const vertex_t* const v) -> vertex_t*;
        static auto high (const vertex_t* const v) -> vertex_t*;
    };

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::TRUE
        () -> bdd
    {
        vertex_t* const trueLeaf {new vertex_t {1, 1}};
        std::map<const vertex_t*, log_val_t> leafValMap
        {
            {trueLeaf, 1}
        };
        
        return bdd {trueLeaf, 0, std::move(leafValMap)};
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::FALSE
        () -> bdd
    {
        vertex_t* const falseLeaf {new vertex_t {1, 1}};
        std::map<const vertex_t*, log_val_t> leafValMap
        {
            {falseLeaf, 0}
        };

        return bdd {falseLeaf, 0, std::move(leafValMap)};
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::VARIABLE
        (const size_t index) -> bdd
    {
        vertex_t* const falseLeaf {new vertex_t {1, index + 1}};
        vertex_t* const trueLeaf  {new vertex_t {2, index + 1}};
        vertex_t* const varVertex {new vertex_t {3, index, {arc_t {falseLeaf}, arc_t {trueLeaf}}}};
        std::map<const vertex_t*, log_val_t> leafValMap
        {
            {falseLeaf, 0}
          , {trueLeaf, 1}
        };

        return bdd {varVertex, index, std::move(leafValMap)};
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
                                , const index_t pVariableCount
                                , std::map<const vertex_t*, log_val_t>&& pLeafToVal) :
        root          {pRoot}
      , variableCount {pVariableCount}  
      , leafToVal     {std::move(pLeafToVal)}
    {
    }

    template<class VertexData, class ArcData>
    bdd<VertexData, ArcData>::~bdd()
    {
        if (this->root)
        {
            std::vector<vertex_t*> toDelete;

            this->traverse(this->root, [&toDelete](vertex_t* const  v) {
                toDelete.push_back(v);
            });

            for (vertex_t* v : toDelete)
            {
                delete v;
            }        
        }
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::operator= (bdd&& rhs) -> bdd&
    {
        this->root          = rhs.root;
        this->variableCount = rhs.variableCount;
        this->leafToVal     = std::move(rhs.leafToVal);

        rhs.root = nullptr;

        return *this;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::to_dot_graph 
        () const -> std::string
    {
        std::ostringstream finalGraphOstr;

        std::vector< std::vector<const vertex_t*> > levelGroups(this->variableCount + 2);
        std::vector< std::tuple<id_t, id_t, bool> > arcs;

        this->traverse(this->root, [&](const vertex_t* const v) {
            if (! v->is_leaf())
            {
                arcs.push_back(std::make_tuple( v->id
                                              , bdd::low(v)->id
                                              , false));

                arcs.push_back(std::make_tuple( v->id
                                              , bdd::high(v)->id
                                              , true));                                              
            }

            levelGroups[v->index].push_back(v);
        });

        finalGraphOstr << "digraph D {" << utils::EOL;

    // node shape
        finalGraphOstr << "    "
                       << "node [shape = square] ";
        for (auto& kvpair : this->leafToVal)
        {
            finalGraphOstr << kvpair.first->id << ' ';
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
                    level != this->leaf_index() ? ("x" + std::to_string(level))
                                                : log_val_to_string(this->leafToVal.at(v))
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
                           << std::to_string(from) 
                           << " -> "
                           << std::to_string(to)
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
    auto bdd<VertexData, ArcData>::get_value 
        (const input_t input) const -> log_val_t
    {
        const std::bitset<sizeof(input_t)> inputBitSet {input};

        vertex_t* currentVertex {this->root};

        while (! this->is_leaf(currentVertex))
        {
            const size_t    bitIndex {this->variableCount - currentVertex->level};
            const log_val_t variableValue {inputBitSet[bitIndex]};
            currentVertex = currentVertex->forwardStar[variableValue].target;
        }
        
        return this->leafToVal.at(currentVertex);
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::value 
        (const vertex_t* const v) const -> log_val_t
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
        return this->variableCount + 1;
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

        if (v->mark != bdd::low(v)->mark)
        {
            this->traverse(bdd::low(v), f);
        }

        if (v->mark != bdd::high(v)->mark)
        {
            this->traverse(bdd::high(v), f);
        }
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::low
        (const vertex_t* const v) -> vertex_t*
    {
        return v->forwardStar[0].target;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::high
        (const vertex_t* const v) -> vertex_t*
    {
        return v->forwardStar[1].target;
    }



    template<class VertexData = int8_t, class ArcData = int8_t>
    auto x (const size_t index) -> bdd<VertexData, ArcData>
    {
        return bdd<VertexData, ArcData>::VARIABLE(index);
    }
}

#endif