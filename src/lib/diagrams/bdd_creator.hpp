#ifndef MIX_DD_BDD_CREATOR_HPP
#define MIX_DD_BDD_CREATOR_HPP

#include "mdd_creator.hpp"
#include "mdd_manipulator.hpp"
#include "pla_file.hpp"

#include <initializer_list>
#include <array>

namespace mix::dd
{
    enum class fold_e {left, tree};

    template<class VertexData, class ArcData>
    class bdd_creator : public mdd_creator<VertexData, ArcData, 2>
    {
    public:
        using base          = mdd_creator<VertexData, ArcData, 2>;
        using manager_t     = typename base::manager_t;
        using bdd_t         = typename base::mdd_t;
        using bool_t        = typename base::log_t;
        using bdd_v         = std::vector<bdd_t>;
        using bool_var_v    = std::vector<bool_var>;
        using manipulator_t = mdd_manipulator<VertexData, ArcData, 2>;

    public:
        bdd_creator (manager_t* const manager, manipulator_t manipulator);

        auto just_var_not (index_t const i)        -> bdd_t;
        auto just_vars    (bool_var_v const& vars) -> bdd_v;
        auto from_pla     (pla_file const& file, fold_e const mm = fold_e::tree) -> bdd_v;

    private:
        auto line_to_product (pla_line const& line)   -> bdd_t;
        auto or_merge        (bdd_v diagrams, fold_e) -> bdd_t;

    private:
        manipulator_t manipulator_;
        manipulator_t orManipulator_;
        manipulator_t andManipulator_;
    };

    template<class VertexData, class ArcData>
    bdd_creator<VertexData, ArcData>::bdd_creator
        (manager_t* const manager, manipulator_t manipulator) :
        base {manager},
        manipulator_    {manipulator},
        orManipulator_  {manager},
        andManipulator_ {manager}
    {
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::just_var_not
        (index_t const i) -> bdd_t
    {
        auto constexpr leafVals = std::array {1, 0};
        return base::just_var_impl(i, leafVals);
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::just_vars
        (bool_var_v const& vars) -> bdd_v
    {
        return utils::map(vars, [this](auto const var)
        {
            return var.complemented ? this->just_var_not(var.index)
                                    : this->just_var(var.index);
        });
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::from_pla
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
                productDiagrams.emplace_back(this->just_val(0));
            }

                orManipulator_.memo_.clear(); // Splay strom?
            // Then merge products using OR.
            functionDiagrams.emplace_back(this->or_merge(std::move(productDiagrams), mm));
                orManipulator_.memo_.clear();
        }

        return functionDiagrams;
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::line_to_product
        (pla_line const& line) -> bdd_t
    {
        auto const vars = cube_to_bool_vars(line.cube);
        return vars.empty() ? this->just_val(0)
                            : andManipulator_.left_fold(this->just_vars(vars), AND());
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::or_merge
        (bdd_v diagrams, fold_e mm) -> bdd_t
    {
        switch (mm)
        {
            case fold_e::tree:
                return orManipulator_.tree_fold(std::move(diagrams), OR());

            case fold_e::left:
                return orManipulator_.left_fold(std::move(diagrams), OR());

            default:
                throw std::runtime_error {"Non-exhaustive enum switch."};
        }
    }
}

#endif