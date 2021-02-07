#ifndef MIX_DD_bdd_manager_HPP
#include "../bdd_manager.hpp"
#endif

namespace mix::dd
{
    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::variable_not
        (index_t const i) -> bdd_t
    {
        auto constexpr leafVals = std::array {1u, 0u};
        return base::variable_impl(i, leafVals);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::operator()
        (index_t const i, NOT) -> bdd_t
    {
        return this->variable_not(i);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::variables
        (bool_var_v const& vars) -> bdd_v
    {
        return utils::fmap(vars, [this](auto const var)
        {
            return var.complemented ? this->variable_not(var.index)
                                    : this->variable(var.index);
        });
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::product
        (bool_var_v const& vars) -> bdd_t
    {
        return this->product(std::begin(vars), std::end(vars));
    }

    template<class VertexData, class ArcData>
    template<class BidirIt>
    auto bdd_manager<VertexData, ArcData>::product
        (BidirIt first, BidirIt last) -> bdd_t
    {
        auto const falseLeaf = base::vertexManager_.terminal_vertex(0);
        auto const trueLeaf  = base::vertexManager_.terminal_vertex(1);
        auto current         = std::prev(last);
        auto prevVertex      = current->complemented
                                   ? base::vertexManager_.internal_vertex(current->index, {trueLeaf, falseLeaf})
                                   : base::vertexManager_.internal_vertex(current->index, {falseLeaf, trueLeaf});
        do
        {
            std::advance(current, -1);
            prevVertex = current->complemented
                             ? base::vertexManager_.internal_vertex(current->index, {prevVertex, falseLeaf})
                             : base::vertexManager_.internal_vertex(current->index, {falseLeaf, prevVertex});

        } while (current != first);

        return bdd_t {prevVertex};
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::from_pla
        (pla_file const& file, fold_e const mm) -> bdd_v
    {
        auto const& plaLines      = file.get_lines();
        auto const  lineCount     = file.line_count();
        auto const  functionCount = file.function_count();

        // Create a diagram for each function.
        auto functionDiagrams = utils::vector<bdd_t>(functionCount);
        for (auto fi = 0u; fi < functionCount; ++fi)
        {
            // First create a diagram for each product.
            auto productDiagrams = utils::vector<bdd_t>(lineCount); // TODO outside of loop and clear?
            for (auto li = 0u; li < lineCount; ++li)
            {
                // We are doing SOP so we are only interested in functions with value 1.
                if (plaLines[li].fVals.at(fi) == 1)
                {
                    productDiagrams.emplace_back(this->line_to_product(plaLines[li]));
                }
            }

            // In this case we just have a constant function.
            if (productDiagrams.empty())
            {
                productDiagrams.emplace_back(this->constant(0));
            }

            // Then merge products using OR.
            functionDiagrams.emplace_back(this->or_merge(productDiagrams, mm));

            this->clear_cache();
            this->collect_garbage();
        }

        return functionDiagrams;
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::line_to_product
        (pla_line const& line) -> bdd_t
    {
        auto const vars  = cube_to_bool_vars(line.cube);
        // auto varDiagrams = this->variables(vars);
        return vars.empty() ? this->constant(0)
                            // : this->template tree_fold<AND>(std::begin(varDiagrams), std::end(varDiagrams));
                            : this->product(vars);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::or_merge
        (bdd_v& diagrams, fold_e mm) -> bdd_t
    {
        switch (mm)
        {
            case fold_e::tree:
                return this->template tree_fold<OR>(std::begin(diagrams), std::end(diagrams));

            case fold_e::left:
                return this->template left_fold<OR>(std::begin(diagrams), std::end(diagrams));

            default:
                throw std::runtime_error {"Non-exhaustive enum switch."};
        }
    }
}