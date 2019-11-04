#ifndef MIX_DD_BDD
#define MIX_DD_BDD

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <bitset>
#include <sstream>
#include "graph.hpp"
#include "typedefs.hpp"

namespace mix::dd
{   
    template<class VertexData, class ArcData>
    class bdd_creator;

    template<class VertexData, class ArcData>
    class bdd_merger;

    template<class VertexData, class ArcData>
    class bdd
    {
    private:
        using vertex = typename graph<VertexData, ArcData>::vertex;
        using arc    = typename graph<VertexData, ArcData>::arc;

    private:    
        vertex* root;
        size_t variableCount;
        std::map<vertex*, log_val_t> leafToVal;
        // TODO map interface ano ale za tým by stačil list

    public:
        friend class bdd_creator<VertexData, ArcData>;
        friend class bdd_merger<VertexData, ArcData>;

    public:
        static auto TRUE  () -> bdd;
        static auto FALSE () -> bdd;

    public:
        bdd(const bdd& other); // not yet implemented
        ~bdd();

        auto to_dot_graph () const -> std::string;
        
        // TODO na vstupe asi bitset keby mala funkcia > ako 64 premenných
        auto get_value (const input_t input) const -> log_val_t;

        // template<class BinaryBoolOperator>
        // auto apply (const bdd& other) const -> bdd;

    private:
        bdd(vertex* pRoot
          , size_t pVariableCount
          , std::map<vertex*, log_val_t>&& pLeafToVal);

        auto reduce () -> void;  // not yet implemented

        auto value   (const vertex* const v) const -> log_val_t;
        auto is_leaf (const vertex* const v) const -> bool;

        template<class UnaryFunction>
        auto traverse (vertex* const v, UnaryFunction f) const -> void;
    };

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::TRUE
        () -> bdd
    {
        vertex* const trueLeaf {new vertex {1, 1}};
        std::map<vertex*, log_val_t> leafValMap
        {
            {trueLeaf, 1}
        };
        
        return bdd {trueLeaf, 0, std::move(leafValMap)};
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::FALSE
        () -> bdd
    {
        vertex* const falseLeaf {new vertex {1, 1}};
        std::map<vertex*, log_val_t> leafValMap
        {
            {falseLeaf, 0}
        };

        return bdd {falseLeaf, 0, std::move(leafValMap)};
    }

    template<class VertexData, class ArcData>
    bdd<VertexData, ArcData>::bdd(vertex* pRoot
                                , size_t pVariableCount
                                , std::map<vertex*, log_val_t>&& pLeafToVal) :
        root          {pRoot}
      , variableCount {pVariableCount}  
      , leafToVal     {std::move(pLeafToVal)}
    {
    }

    template<class VertexData, class ArcData>
    bdd<VertexData, ArcData>::~bdd()
    {
        std::vector<vertex*> toDelete;

        this->traverse(this->root, [&toDelete](vertex* const  v) {
            toDelete.push_back(v);
        });

        for (vertex* v : toDelete)
        {
            delete v;
        }        
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::to_dot_graph 
        () const -> std::string
    {
        std::ostringstream graphOst;
        std::ostringstream arcOst;
        std::ostringstream vertexOst;
        std::vector<std::ostringstream> levels;
        levels.resize(this->variableCount + 2);

        for (auto& levelOst : levels)
        {
            levelOst << "    {rank = same; ";
        }

        this->traverse(this->root, [&](vertex* const v) {
            if (! v->is_leaf())
            {
                vertex* const negativeTarget {v->forwardStar[0].target};
                vertex* const positiveTarget {v->forwardStar[1].target};

                vertexOst << "    " 
                          << std::to_string(v->id) 
                          << " [label = " << ('x' + std::to_string(v->level) ) << "];" 
                          << '\n';

                arcOst << "    " << v->id << " -> " << negativeTarget->id << " [style = dashed];" << '\n';
                arcOst << "    " << v->id << " -> " << positiveTarget->id << " [style = solid];"  << '\n';
            }
            else
            {
                vertexOst << "    " 
                          << std::to_string(v->id) 
                          << " [label = " << std::to_string(this->leafToVal.at(v)) << "];" 
                          << '\n';
            }

            levels[v->level] << std::to_string(v->id) << "; ";
        });

        graphOst << "digraph D {"                    << '\n'
                 << "    node [shape = square] 1 2;" << '\n'
                 << "    node [shape = circle];"     << "\n\n"
                 << vertexOst.str() << '\n'
                 << arcOst.str()    << '\n';

        for (auto& levelOst : levels)
        {
            levelOst << "}" << '\n';
            graphOst << levelOst.str();
        }

        graphOst << '}' << '\n';

        return graphOst.str();
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::get_value 
        (const input_t input) const -> log_val_t
    {
        const std::bitset<sizeof(input_t)> inputBitSet {input};

        vertex* currentVertex {this->root};

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
        (const vertex* const v) const -> log_val_t
    {
        return this->is_leaf(v) ? this->leafToVal(v) : X;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::is_leaf
        (const vertex* const v) const -> bool
    {
        return v->level == this->variableCount + 1;
    }

    template<class VertexData, class ArcData>
    template<class UnaryFunction>
    auto bdd<VertexData, ArcData>::traverse 
        (vertex* const v, UnaryFunction f) const -> void
    {
        v->mark = ! v->mark;

        f(v);

        if (this->is_leaf(v))
        {
            return;
        }

        if (v->mark != v->forwardStar[0].target->mark)
        {
            this->traverse(v->forwardStar[0].target, f);
        }

        if (v->mark != v->forwardStar[1].target->mark)
        {
            this->traverse(v->forwardStar[1].target, f);
        }
    }
}

#endif