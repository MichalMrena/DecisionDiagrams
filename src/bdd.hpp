#ifndef MIX_DD_BDD
#define MIX_DD_BDD

#include <string>
#include <vector>
#include <map>
#include <bitset>
#include <queue>
#include <set>
#include "graph.hpp"
#include "typedefs.hpp"

namespace mix::dd
{   
    template<class VertexData, class ArcData>
    class bdd
    {
    private:
        using vertex = typename graph<VertexData, ArcData>::vertex;
        using arc    = typename graph<VertexData, ArcData>::arc;

        vertex* root;
        size_t variableCount;

        // TODO map interface ano ale za tým by stačil list
        // TODO is leaf podla indexu netreba iterovat
        std::map<vertex*, log_val_t> leafToVal;

    public:
        template<typename V, typename A>
        friend class bdd_creator;

    public:
        static auto TRUE  () -> bdd; // not yet implemented
        static auto FALSE () -> bdd; // not yet implemented

    public:
        bdd(const bdd& other); // not yet implemented
        ~bdd();

        auto to_dot_graph () const -> std::string;
        
        // TODO na vstupe asi bitset keby mala funkcia > ako 64 premenných
        auto get_value (const input_t input) const -> log_val_t;

        template<class BinaryBoolOperator> // TODO enableIf is binary boolean operator
        auto apply (const bdd& other) const -> bdd; // not yet implemented

    private:
        bdd(vertex* pRoot
          , size_t pVariableCount
          , std::map<vertex*, log_val_t>&& pLeafToVal);

        auto reduce () -> void;  // not yet implemented

        auto value   (vertex* const v) const -> log_val_t;
        auto is_leaf (vertex* const v) const -> bool;  // not yet implemented

        template<class UnaryFunction>
        auto traverse (vertex* const v, UnaryFunction f) const -> void;
    };

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
        // TODO foreach metodu využívajúcu mark vo vrcholoch
        std::set<vertex*> processed;
        std::queue<vertex*> toProcess;
        toProcess.push(this->root);

        while (! toProcess.empty())
        {
            vertex* v {toProcess.front()};
            toProcess.pop();

            if (processed.find(v) != processed.end())
            {
                continue;
            }

            for (arc& a : v->forwardStar)
            {
                if (a.target)
                {
                    toProcess.push(a.target);
                }
            }
            
            processed.insert(v);
        }

        for (vertex* v : processed)
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
        std::vector<std::string> sameLevel;  // TODO      

        graphOst << "digraph D {" << '\n'
                 << "    node [shape = square] 1 2;" << '\n'
                 << "    node [shape = circle];"     << "\n\n";

        this->traverse(this->root, [&](vertex* const v) {
            if (! v->is_leaf())
            {
                vertex* const negativeTarget {v->forwardStar[0].target};
                vertex* const positiveTarget {v->forwardStar[1].target};

                vertexOst << "    " 
                          << std::to_string(v->id) 
                          << " [label=" << ('x' + std::to_string(v->level) ) << "];" 
                          << '\n';

                arcOst << "    " << v->id << " -> " << negativeTarget->id << " [style = dashed];" << '\n';
                arcOst << "    " << v->id << " -> " << positiveTarget->id << " [style = solid];"  << '\n';
            }
            else
            {
                vertexOst << "    " 
                          << std::to_string(v->id) 
                          << " [label=" << std::to_string(this->leafToVal.at(v)) << "];" 
                          << '\n';
            }
        });

        graphOst << vertexOst.str();
        graphOst << arcOst.str();
        graphOst << '}' << '\n';

        return graphOst.str();
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::get_value 
        (const input_t input) const -> log_val_t
    {
        const std::bitset<sizeof(input_t)> inputBitSet {input};

        vertex* currentVertex {this->root};

        // const size_t variableCount {this->valToLeaf.at(0)->level};

        // while (! currentVertex->is_leaf())
        // {
        //     const size_t    bitIndex {variableCount - currentVertex->level - 1};
        //     const log_val_t variableValue {inputBitSet[bitIndex]};
        //     currentVertex = currentVertex->forwardStar[variableValue].target;
        // }
        
        return this->leafToVal.at(currentVertex);
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::value 
        (vertex* const v) const -> log_val_t
    {
        return v->is_leaf() ? this->leafToVal(v) : X;
    }

    template<class VertexData, class ArcData>
    template<class UnaryFunction>
    auto bdd<VertexData, ArcData>::traverse 
        (vertex* const v, UnaryFunction f) const -> void
    {
        v->mark = ! v->mark;

        f(v);

        if (v->is_leaf())
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