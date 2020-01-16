#ifndef _MIX_DD_BDDS_FROM_PLA_
#define _MIX_DD_BDDS_FROM_PLA_

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include "bdd.hpp"
#include "bdd_merger.hpp"
#include "bdd_creator.hpp"
#include "pla_file.hpp"
#include "../dd/graph.hpp"
#include "../dd/typedefs.hpp"

namespace mix::dd
{
// declarations:

    template<class VertexData, class ArcData>
    class bdds_from_pla
    {
    public:
        using bdd_t          = bdd<VertexData, ArcData>;
        using leaf_val_map_t = typename bdd_t::leaf_val_map;
        using vertex_t       = vertex<VertexData, ArcData, 2>;
        using arc_t          = arc<VertexData, ArcData, 2>;

    public:
        auto create (const pla_file& file) -> std::vector<bdd_t>;

    // TMP later private
    public: 
        auto or_merge_diagrams (std::vector<bdd_t> diagrams) -> bdd_t;
    };

// definitions:

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::create 
        (const pla_file& file) -> std::vector<bdd_t>
    {
        using creator_t = bdd_creator<VertexData, ArcData>;

        // It is important to resize both vectors at the begining.
        // Otherwise expensive copies would have been made during the construction. 
        std::vector< std::vector<bdd_t> > subDiagrams(file.function_count());
        for (auto& subVector : subDiagrams)
        {
            subVector.reserve(file.line_count());
        }

        const auto& plaLines      {file.get_lines()};
        const auto  lineCount     {file.line_count()};
        const auto  functionCount {file.function_count()};

        creator_t creator;

        // https://stackoverflow.com/questions/13357065/how-does-openmp-handle-nested-loops
        // parallelizable for
        for (int32_t li {0}; li < lineCount; ++li)
        {
            // parallelizable for
            for (int32_t fi {0}; fi < functionCount; ++fi)
            {
                subDiagrams[fi].emplace_back(
                    creator.create_simple( plaLines[li].varVals, plaLines[li].fVals[fi] )
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
                    (*dst)[i] = std::move((*src)[i << 1]);
                }
            }

            std::swap(src, dst);
        }

        return bdd_t {std::move(src->front())};
    }
}

#endif