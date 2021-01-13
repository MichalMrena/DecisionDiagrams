#ifndef MIX_DD_bdd_manager_HPP
#include "../bdd_manager.hpp"
#endif

namespace mix::dd
{
    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::variable_not
        (index_t const i) -> bdd_t
    {
        auto constexpr leafVals = std::array {1, 0};
        return base::just_var_impl(i, leafVals);
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
        return utils::map(vars, [this](auto const var)
        {
            return var.complemented ? this->variable_not(var.index)
                                    : this->variable(var.index);
        });
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
            auto productDiagrams = utils::vector<bdd_t>(lineCount);
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
        }

        return functionDiagrams;
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::line_to_product
        (pla_line const& line) -> bdd_t
    {
        auto const vars = cube_to_bool_vars(line.cube);
        return vars.empty() ? this->constant(0)
                            : this->left_fold(this->variables(vars), AND());
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::or_merge
        (bdd_v& diagrams, fold_e mm) -> bdd_t
    {
        switch (mm)
        {
            case fold_e::tree:
                return this->tree_fold(diagrams, OR());

            case fold_e::left:
                return this->left_fold(diagrams, OR());

            default:
                throw std::runtime_error {"Non-exhaustive enum switch."};
        }
    }
}