#ifndef _MIX_DD_BDDS_FROM_PLA_
#define _MIX_DD_BDDS_FROM_PLA_

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include "typedefs.hpp"
#include "bdd.hpp"
#include "graph.hpp"
#include "pla_file.hpp"

namespace mix::dd
{
// declarations:

    template<class VertexData, class ArcData>
    class bdds_from_pla
    {
    public:
        using bdd_t    = bdd<VertexData, ArcData>;
        using vertex_t = vertex<VertexData, ArcData, 2>;
        using arc_t    = arc<VertexData, ArcData, 2>;

    public:
        auto create (const pla_file& file) -> std::vector<bdd_t>;

    // TMP later private
    public: 
        auto create_diagram ( const std::vector<log_val_t>& varVals
                            , const log_val_t fVal) -> bdd_t;
        
        auto or_merge_diagrams (std::vector<bdd_t> diagrams) -> bdd_t;
    };

// definitions:

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::create 
        (const pla_file& file) -> std::vector<bdd_t>
    {
        std::vector< std::vector<bdd_t> > subDiagrams(file.function_count());

        const auto& plaLines      {file.get_lines()};
        const auto  lineCount     {file.line_count()};
        const auto  functionCount {file.function_count()};

        // https://stackoverflow.com/questions/13357065/how-does-openmp-handle-nested-loops
        // parallelizable for
        for (int32_t li {0}; li < lineCount; ++li)
        {
            // parallelizable for
            for (int32_t fi {0}; fi < functionCount; ++fi)
            {
                subDiagrams[fi].emplace_back(
                    this->create_diagram( plaLines[li].varVals, plaLines[li].fVals[fi] )
                );
            }
        }
        
        std::vector<bdd_t> finalDiagrams(file.function_count());
        
        // parallelizable for
        for (int32_t fi {0}; fi < functionCount; ++fi)
        {
            finalDiagrams[fi] = this->or_merge_diagrams(std::move(subDiagrams[fi]));
        }
        
        return finalDiagrams;
    }

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::create_diagram
        (const std::vector<log_val_t>& varVals, const log_val_t fVal) -> bdd_t
    {
        if (0 == fVal)
        {
            return bdd_t::FALSE();
        }

        const index_t leafLevel {static_cast<index_t>(varVals.size() + 1)};
        index_t index  {1};
        id_t    nextId {1};

        std::vector<vertex_t*> relevantVariables;
        relevantVariables.reserve(varVals.size());

        for (auto val : varVals)
        {
            if (val != X)
            {
                relevantVariables.push_back(new vertex_t {nextId++, index});
            }

            ++index;
        }

        if (relevantVariables.empty())
        {
            throw std::runtime_error {"Invalid pla line."};
        }

        auto valLeaf {new vertex_t {nextId++, leafLevel}};
        auto xLeaf   {new vertex_t {nextId++, leafLevel}};

        auto b {relevantVariables.begin()};
        auto e {relevantVariables.end()};
        --e;

        while (b != e)
        {
            const auto v      {*b};
            const auto varVal {varVals[v->index - 1]};
            
            ++b;
            
            v->forwardStar[varVal].target  = *b;
            v->forwardStar[!varVal].target = xLeaf;
        }

        const auto v      {*b};
        const auto varVal {varVals[v->index - 1]};

        v->forwardStar[varVal].target  = valLeaf;
        v->forwardStar[!varVal].target = xLeaf;
        
        std::map<const vertex_t*, log_val_t> leafToVal 
        { 
            {valLeaf, fVal}
          , {xLeaf, 0} 
        };

        return bdd_t 
        {
            relevantVariables.front()
          , leafLevel - 1
          , std::move(leafToVal)
        };
    }

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::or_merge_diagrams
        (std::vector<bdd_t> diagrams) -> bdd_t
    {
        const auto numOfSteps 
        {
            static_cast<size_t>(std::ceil(std::log2(diagrams.size())))
        };

        auto diagramCount 
        {
            static_cast<int32_t>(diagrams.size())
        };

        std::vector<bdd_t> auxDiagrams((diagrams.size() >> 1) + 1);
        std::vector<bdd_t>* src {&diagrams};
        std::vector<bdd_t>* dst {&auxDiagrams};

        for (size_t step {0}; step < numOfSteps; ++step)
        {
            const bool justMoveLast {diagramCount & 1};
            
            diagramCount = (diagramCount >> 1) + (diagramCount & 1);

            // parallelizable for
            for (int32_t i {0}; i < diagramCount; ++i)
            {
                if (i < diagramCount - 1 || !justMoveLast)
                {
                    (*dst)[i] = (*src)[i << 1] || (*src)[(i << 1) + 1];
                }
                else
                {
                    (*dst)[i] = std::move((*src)[i]);
                }
            }

            std::swap(src, dst);
        }

        std::swap(src, dst);

        return bdd_t {std::move(dst->front())};
    }
}

#endif