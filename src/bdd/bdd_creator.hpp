#ifndef _MIX_DD_BDD_CREATOR_
#define _MIX_DD_BDD_CREATOR_

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iterator>
#include "bdd.hpp"
#include "bdd_manipulator.hpp"
#include "pla_file.hpp"
#include "../dd/graph.hpp"
#include "../dd/dd_manipulator_base.hpp"

namespace mix::dd
{
    enum merge_mode_e {sequential, iterative};

    template<class VertexData, class ArcData>
    class bdd_creator : public dd_manipulator_base<VertexData, ArcData, 2>
    {
    public:
        using bdd_t        = bdd<VertexData, ArcData>;
        using leaf_val_map = typename bdd_t::leaf_val_map;
        using vertex_t     = typename bdd_t::vertex_t;
        using arc_t        = typename bdd_t::arc_t;
        using arc_arr_t    = typename vertex_t::forward_star_arr;

        auto just_val (const bool_t  value) -> bdd_t;
        auto just_var (const index_t index) -> bdd_t;

        template<class InputIt>
        auto create_product (InputIt varsBegin, InputIt varsEnd, const bool_t fVal) -> bdd_t;

        auto create_from_pla   (const pla_file& file, merge_mode_e mm) -> std::vector<bdd_t>;
        auto create_from_pla_p (const pla_file& file, merge_mode_e mm) -> std::vector<bdd_t>;

        virtual ~bdd_creator() = default;

    private: 
        auto or_merge_iterative  (std::vector<bdd_t> diagrams) -> bdd_t;
        auto or_merge_sequential (std::vector<bdd_t> diagrams) -> bdd_t;
    };

    template<class VertexData = double, class ArcData = empty_t>
    auto x (const index_t i) -> bdd<VertexData, ArcData>
    {
        bdd_creator<VertexData, ArcData> creator;
        return creator.just_var(i);
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::just_val
        (const bool_t value) -> bdd_t
    {
        const auto leaf {this->create_vertex(0, 0u)};
       
        leaf_val_map leafValMap
        {
            {leaf, value}
        };

        return bdd_t {leaf, 0, std::move(leafValMap)};
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::just_var
        (const index_t index) -> bdd_t
    {
        const auto falseLeaf {this->create_vertex(0, index + 1)};
        const auto trueLeaf  {this->create_vertex(1, index + 1)};
        const auto varVertex {this->create_vertex(2, index, arc_arr_t {arc_t {falseLeaf}, arc_t {trueLeaf}})};
        
        leaf_val_map leafValMap
        {
            {falseLeaf, 0}
          , {trueLeaf,  1}
        };

        return bdd_t {varVertex, index + 1, std::move(leafValMap)};
    }

    template<class VertexData, class ArcData>
    template<class InputIt>
    auto bdd_creator<VertexData, ArcData>::create_product
        (InputIt varValsBegin, InputIt varValsEnd, const bool_t fVal) -> bdd_t
    {
        static_assert( std::is_same_v<bool_t, typename std::iterator_traits<InputIt>::value_type>
                     , "Variable values must be of type bool_t" );

        if (1 != fVal)
        {
            return this->just_val(0);
        }

        const auto varCount  {std::distance(varValsBegin, varValsEnd)};
        const auto leafIndex {static_cast<index_t>(varCount)};
        index_t index  {0};
        id_t    nextId {0};

        std::vector<vertex_t*> relevantVariables;
        relevantVariables.reserve(varCount);

        auto varValsIt {varValsBegin};
        while (varValsIt != varValsEnd)
        {
            if (*varValsIt != X)
            {
                relevantVariables.push_back(this->create_vertex(nextId++, index));
            }

            ++index;
            ++varValsIt;
        }

        if (relevantVariables.empty())
        {
            return this->just_val(1);
        }

        auto trueLeaf  {this->create_vertex(nextId++, leafIndex)};
        auto falseLeaf {this->create_vertex(nextId++, leafIndex)};

        auto relevantVarsIt  {relevantVariables.begin()};
        auto relevantVarsEnd {relevantVariables.end()};
        --relevantVarsEnd;

        varValsIt = varValsBegin; // TODO moc škaredé
        while (relevantVarsIt != relevantVarsEnd)
        {
            const auto varIndex {std::distance(varValsBegin, varValsIt)};
            const auto vertex   {*relevantVarsIt};

            std::advance(varValsIt, vertex->index - varIndex);
            
            const auto varVal {*varValsIt};
            
            ++relevantVarsIt;
            
            vertex->son(varVal)  = *relevantVarsIt;
            vertex->son(!varVal) = falseLeaf;
        }

        const auto varIndex {std::distance(varValsBegin, varValsIt)};
        const auto vertex   {*relevantVarsIt};
        std::advance(varValsIt, vertex->index - varIndex);
        const auto varVal   {*varValsIt};
        vertex->son(varVal)  = trueLeaf;
        vertex->son(!varVal) = falseLeaf;

        leaf_val_map leafToVal 
        { 
            {trueLeaf,  1}
          , {falseLeaf, 0} 
        };

        return bdd_t 
        {
            relevantVariables.front()
          , static_cast<index_t>(varCount)
          , std::move(leafToVal)
        };
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::create_from_pla 
        (const pla_file& file, merge_mode_e mm) -> std::vector<bdd_t>
    {
        const auto& plaLines      {file.get_lines()};
        const auto  lineCount     {file.line_count()};
        const auto  functionCount {file.function_count()};

        std::vector<bdd_t> finalDiagrams(functionCount);

        for (int32_t fi = 0; fi < functionCount; ++fi)
        {
            std::vector<bdd_t> diagrams;
            diagrams.reserve(lineCount);

            for (int32_t li = 0; li < lineCount; ++li)
            {
                if (plaLines[li].fVals.at(fi) == 1)
                {
                    diagrams.emplace_back(
                    this->create_product( plaLines[li].cube.begin()
                                        , plaLines[li].cube.end(), 1 ));
                }
            }

            if (merge_mode_e::sequential == mm) finalDiagrams[fi] = this->or_merge_sequential(diagrams);
            if (merge_mode_e::iterative  == mm) finalDiagrams[fi] = this->or_merge_iterative(diagrams);
            
            finalDiagrams[fi].set_labels(file.get_input_labels());
        }
        
        return finalDiagrams;
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::create_from_pla_p 
        (const pla_file& file, merge_mode_e mm) -> std::vector<bdd_t>
    {
        const auto& plaLines      {file.get_lines()};
        const auto  lineCount     {file.line_count()};
        const auto  functionCount {file.function_count()};

        std::vector<bdd_t> finalDiagrams(functionCount);

        #pragma omp parallel for schedule(dynamic, 1)
        for (int32_t fi = 0; fi < functionCount; ++fi)
        {
            std::vector<bdd_t> diagrams;
            diagrams.reserve(lineCount);

            for (int32_t li = 0; li < lineCount; ++li)
            {
                if (plaLines[li].fVals.at(fi) == 1)
                {
                    diagrams.emplace_back(
                    this->create_product( plaLines[li].cube.begin()
                                        , plaLines[li].cube.end(), 1 ));
                }
            }

            if (merge_mode_e::sequential == mm) finalDiagrams[fi] = this->or_merge_sequential(diagrams);
            if (merge_mode_e::iterative  == mm) finalDiagrams[fi] = this->or_merge_iterative(diagrams);
            
            finalDiagrams[fi].set_labels(file.get_input_labels());
        }
        
        return finalDiagrams;
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::or_merge_iterative
        (std::vector<bdd_t> diagrams) -> bdd_t
    {
        using manipulator_t = bdd_manipulator<VertexData, ArcData>;

        if (diagrams.empty())
        {
            return this->just_val(0);
        }

        const auto numOfSteps 
        {
            static_cast<size_t>(std::ceil(std::log2(diagrams.size())))
        };

        auto diagramCount 
        {
            static_cast<int32_t>(diagrams.size())
        };

        manipulator_t manipulator;
        for (size_t step {0}; step < numOfSteps; ++step)
        {
            const bool justMoveLast {diagramCount & 1};
            
            diagramCount = (diagramCount >> 1) + (diagramCount & 1);

            // TODO bez ifu v cykle...
            for (int32_t i = 0; i < diagramCount; ++i)
            {
                if (i < diagramCount - 1 || !justMoveLast)
                {
                    diagrams[i] = manipulator.apply( std::move(diagrams[2 * i])
                                                   , OR {}
                                                   , std::move(diagrams[2 * i + 1]) );
                }
                else
                {
                    diagrams[i] = std::move(diagrams[2 * i]);
                }
            }

            // if (diagramCount & 1)
            // {
            //     diagrams[diagramCount - 1] = std::move(diagrams[2 * (diagramCount - 1)]);
            // }
        }

        return bdd_t {std::move(diagrams.front())};
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::or_merge_sequential
        (std::vector<bdd_t> diagrams) -> bdd_t
    {
        using manipulator_t = bdd_manipulator<VertexData, ArcData>;

        if (diagrams.empty())
        {
            return this->just_val(0);
        }

        auto it  {diagrams.begin() + 1};
        auto end {diagrams.end()};

        manipulator_t manipulator;

        while (it != end)
        {
            diagrams.front() = manipulator.apply( std::move(diagrams.front())
                                                , OR {}
                                                , std::move(*it) );
            ++it;
        }

        return bdd_t {std::move(diagrams.front())};
    }
}

#endif