#ifndef _MIX_DD_BDDS_FROM_PLA_
#define _MIX_DD_BDDS_FROM_PLA_

#include <iostream>

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include "bdd.hpp"
#include "bdd_merger.hpp"
#include "bdd_creator.hpp"
#include "pla_file.hpp"
#include "pla_heuristic.hpp"
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
        auto create_i  (const pla_file& file) -> std::vector<bdd_t>;
        auto create_s  (const pla_file& file) -> std::vector<bdd_t>;
        auto create_ip (const pla_file& file) -> std::vector<bdd_t>;

        auto create_smart (pla_file file) -> std::vector<bdd_t>;

    private: 
        auto or_merge_iterative (std::vector<bdd_t> diagrams) -> bdd_t;
        auto or_merge_iterative_parallel (std::vector<bdd_t> diagrams) -> bdd_t;
        auto or_merge_sequential (std::vector<bdd_t> diagrams) -> bdd_t;

        auto products_sizes   (const pla_file& file) -> std::vector<size_t>;
        auto all_swap_pairs   (const std::vector<index_t>& is) -> std::vector<std::pair<index_t, index_t>>;
    };

// definitions:
    

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::create_i 
        (const pla_file& file) -> std::vector<bdd_t>
    {
        using creator_t = bdd_creator<VertexData, ArcData>;

        const auto& plaLines      {file.get_lines()};
        const auto  lineCount     {file.line_count()};
        const auto  functionCount {file.function_count()};

        std::vector<bdd_t> finalDiagrams(functionCount);
        creator_t creator;

        for (int32_t fi = 0; fi < functionCount; ++fi)
        {
            std::vector<bdd_t> diagrams;
            diagrams.reserve(lineCount);

            for (int32_t li = 0; li < lineCount; ++li)
            {
                if (plaLines[li].fVals.at(fi) != 1)
                {
                    continue;
                }

                diagrams.emplace_back(
                    creator.create_product( plaLines[li].varVals.begin()
                                          , plaLines[li].varVals.end()
                                          , 1 ));
            }

            finalDiagrams[fi] = this->or_merge_iterative(diagrams);
        }
        
        return finalDiagrams;
    }

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::create_ip 
        (const pla_file& file) -> std::vector<bdd_t>
    {
        using creator_t = bdd_creator<VertexData, ArcData>;

        const auto& plaLines      {file.get_lines()};
        const auto  lineCount     {file.line_count()};
        const auto  functionCount {file.function_count()};

        std::vector<bdd_t> finalDiagrams(functionCount);
        creator_t creator;

        #pragma omp parallel for schedule(dynamic, 1)
        for (int32_t fi = 0; fi < functionCount; ++fi)
        {
            std::vector<bdd_t> diagrams;
            diagrams.reserve(lineCount);

            for (int32_t li = 0; li < lineCount; ++li)
            {
                if (plaLines[li].fVals.at(fi) != 1)
                {
                    continue;
                }

                diagrams.emplace_back(
                    creator.create_product( plaLines[li].varVals.begin()
                                          , plaLines[li].varVals.end()
                                          , 1 ));
            }

            finalDiagrams[fi] = this->or_merge_iterative(diagrams);
        }
        
        return finalDiagrams;
    }

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::create_s 
        (const pla_file& file) -> std::vector<bdd_t>
    {
        using creator_t = bdd_creator<VertexData, ArcData>;

        const auto& plaLines      {file.get_lines()};
        const auto  lineCount     {file.line_count()};
        const auto  functionCount {file.function_count()};

        std::vector<bdd_t> finalDiagrams(functionCount);
        creator_t creator;

        for (int32_t fi = 0; fi < functionCount; ++fi)
        {
            std::vector<bdd_t> diagrams;
            diagrams.reserve(lineCount);

            for (int32_t li = 0; li < lineCount; ++li)
            {
                if (plaLines[li].fVals.at(fi) != 1)
                {
                    continue;
                }

                diagrams.emplace_back(
                    creator.create_product( plaLines[li].varVals.begin()
                                          , plaLines[li].varVals.end()
                                          , 1 ));
            }

            finalDiagrams[fi] = this->or_merge_sequential(diagrams);
        }
        
        return finalDiagrams;
    }

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::create_smart
        (pla_file file) -> std::vector<bdd_t>
    {
        return this->create_i(improve_ordering(file));
    }

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::or_merge_iterative
        (std::vector<bdd_t> diagrams) -> bdd_t
    {
        using merger_t  = bdd_merger<VertexData, ArcData>;
        using creator_t = bdd_creator<VertexData, ArcData>;

        if (diagrams.empty())
        {
            return creator_t::just_false();
        }

        const auto numOfSteps 
        {
            static_cast<size_t>(std::ceil(std::log2(diagrams.size())))
        };

        auto diagramCount 
        {
            static_cast<int32_t>(diagrams.size())
        };

        merger_t merger;
        for (size_t step {0}; step < numOfSteps; ++step)
        {
            const bool justMoveLast {diagramCount & 1};
            
            diagramCount = (diagramCount >> 1) + (diagramCount & 1);

            for (int32_t i = 0; i < diagramCount; ++i)
            {
                if (i < diagramCount - 1 || !justMoveLast)
                {
                    diagrams[i] = merger.merge_recycling( std::move(diagrams[i << 1])
                                                        , std::move(diagrams[(i << 1) + 1])
                                                        , OR {} );
                }
                else
                {
                    diagrams[i] = std::move(diagrams[i << 1]);
                }
            }
        }

        return bdd_t {std::move(diagrams.front())};
    }

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::or_merge_iterative_parallel
        (std::vector<bdd_t> diagrams) -> bdd_t
    {
        using merger_t  = bdd_merger<VertexData, ArcData>;
        using creator_t = bdd_creator<VertexData, ArcData>;

        if (diagrams.empty())
        {
            return creator_t::just_false();
        }

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

            #pragma omp parallel for
            for (int32_t i = 0; i < diagramCount; ++i)
            {
                merger_t merger;
                auto dstptr {dst->data() + i};
                if (i < diagramCount - 1 || !justMoveLast)
                {
                    *dstptr = merger.merge_reduced( std::move((*src)[i << 1])
                                                  , std::move((*src)[(i << 1) + 1])
                                                  , OR {} );
                }
                else
                {
                    *dstptr = std::move((*src)[i << 1]);
                }
            }

            std::swap(src, dst);
        }

        return bdd_t {std::move(src->front())};
    }

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::or_merge_sequential
        (std::vector<bdd_t> diagrams) -> bdd_t
    {
        using merger_t  = bdd_merger<VertexData, ArcData>;
        using creator_t = bdd_creator<VertexData, ArcData>;

        if (diagrams.empty())
        {
            return creator_t::just_false();
        }

        auto it  {diagrams.begin() + 1};
        auto end {diagrams.end()};

        merger_t merger;

        while (it != end)
        {
            diagrams.front() = merger.merge(diagrams.front(), *it, OR {});
            ++it;
        }

        return bdd_t {std::move(diagrams.front())};
    }
}

#endif