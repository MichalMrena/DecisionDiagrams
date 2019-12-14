#ifndef _MIX_DD_BDD_
#define _MIX_DD_BDD_

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
    class bdd_reducer;

    template<class VertexData, class ArcData>
    class bdds_from_pla;

    template<class VertexData = def_vertex_data_t
           , class ArcData = def_arc_data_t>
    class bdd
    {
    private:
        using vertex = typename graph<VertexData, ArcData>::vertex;
        using arc    = typename graph<VertexData, ArcData>::arc;

    private:    
        vertex* root {nullptr};
        size_t variableCount {0};
        std::map<const vertex*, log_val_t> leafToVal; // TODO asi by som to tam dal explicitne...
        // TODO map interface ano ale za tým by asi stačila menej monštrózna implementácia

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

        // TODO copy swap
        auto operator= (bdd&& rhs) -> bdd&;

        auto to_dot_graph () const -> std::string;
        
        // TODO na vstupe asi bitset keby mala funkcia > ako 64 premenných
        auto get_value (const input_t input) const -> log_val_t;

    public: // just tmp private later
        bdd(vertex* pRoot
          , size_t pVariableCount
          , std::map<const vertex*, log_val_t>&& pLeafToVal);

        auto value   (const vertex* const v) const -> log_val_t;
        auto is_leaf (const vertex* const v) const -> bool;

        template<class UnaryFunction>
        auto traverse (vertex* const v, UnaryFunction f) const -> void;

        static auto low  (const vertex* const v) -> vertex*;
        static auto high (const vertex* const v) -> vertex*;
    };

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::TRUE
        () -> bdd
    {
        vertex* const trueLeaf {new vertex {1, 1}};
        std::map<const vertex*, log_val_t> leafValMap
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
        std::map<const vertex*, log_val_t> leafValMap
        {
            {falseLeaf, 0}
        };

        return bdd {falseLeaf, 0, std::move(leafValMap)};
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::VARIABLE
        (const size_t index) -> bdd
    {
        vertex* const falseLeaf {new vertex {1, index + 1}};
        vertex* const trueLeaf  {new vertex {2, index + 1}};
        vertex* const varVertex {new vertex {3, index, {arc {falseLeaf}, arc {trueLeaf}}}};
        std::map<const vertex*, log_val_t> leafValMap
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
    bdd<VertexData, ArcData>::bdd(vertex* pRoot
                                , size_t pVariableCount
                                , std::map<const vertex*, log_val_t>&& pLeafToVal) :
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
            std::vector<vertex*> toDelete;

            this->traverse(this->root, [&toDelete](vertex* const  v) {
                toDelete.push_back(v);
            });

            for (vertex* v : toDelete)
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
        std::ostringstream arcOst;
        std::ostringstream vertexLabelOstr;
        std::ostringstream leafShapeOstr;
        std::vector<std::ostringstream> levels;
        levels.resize(this->variableCount + 2);

        leafShapeOstr << "    node [shape = square] ";

        for (auto& levelOst : levels)
        {
            levelOst << "    {rank = same; ";
        }

        this->traverse(this->root, [&](const vertex* const v) {
            if (! v->is_leaf())
            {
                const vertex* const negativeTarget {bdd::low(v)};
                const vertex* const positiveTarget {bdd::high(v)};

                vertexLabelOstr 
                    << "    " 
                    << std::to_string(v->id) 
                    << " [label = " << ('x' + std::to_string(v->level)) << "];" 
                    << '\n';

                arcOst << "    " << v->id << " -> " << negativeTarget->id << " [style = dashed];" << '\n';
                arcOst << "    " << v->id << " -> " << positiveTarget->id << " [style = solid];"  << '\n';
            }
            else
            {
                vertexLabelOstr 
                    << "    " 
                    << std::to_string(v->id) 
                    << " [label = " << log_val_to_char(this->leafToVal.at(v)) << "];" 
                    << '\n';

                leafShapeOstr << v->id << ' ';
            }

            levels[v->level] << std::to_string(v->id) << "; ";
        });

        leafShapeOstr << ';';

        finalGraphOstr 
            << "digraph D {"                << '\n'
            << leafShapeOstr.str()          << '\n'
            << "    node [shape = circle];" << "\n\n"
            << vertexLabelOstr.str()        << '\n'
            << arcOst.str()                 << '\n';

        for (auto& levelOst : levels)
        {
            levelOst << "}" << '\n';
            finalGraphOstr << levelOst.str();
        }

        finalGraphOstr << '}' << '\n';

        return finalGraphOstr.str();
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
        return this->is_leaf(v) ? this->leafToVal.at(v) : X;
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
        (const vertex* const v) -> vertex*
    {
        return v->forwardStar[0].target;
    }

    template<class VertexData, class ArcData>
    auto bdd<VertexData, ArcData>::high
        (const vertex* const v) -> vertex*
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