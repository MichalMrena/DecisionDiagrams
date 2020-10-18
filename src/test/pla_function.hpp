#ifndef MIX_DD_PLA_FUNCTION_
#define MIX_DD_PLA_FUNCTION_

#include "../bdd/bdd.hpp"
#include "../bdd/bdd_creator.hpp"
#include "../bdd/pla_file.hpp"
#include "../bdd/var_vals.hpp"

#include <vector>
#include <functional>

namespace mix::dd
{
    class pla_function
    {
    public:
        using bdd_t = bdd<double, void>;

    public:
        static auto from_file (pla_file const& file) -> pla_function;

        template< class BoolFunctionInput
                , class GetVarVal = get_var_val<BoolFunctionInput> >
        auto get_value (BoolFunctionInput const& input, std::size_t const i) const -> bool_t;

    private:
        using product_v  = std::vector<bdd_t>;
        using function_v = std::vector<product_v>;
    
    private:
        pla_function(function_v functions);

    private:
        function_v functions_;
    };

    inline auto pla_function::from_file
        (pla_file const& file) -> pla_function
    {
        using creator_t = bdd_creator<double, void>;
        auto creator   = creator_t {};
        auto functions = function_v(file.function_count());

        for (auto fi = 0u; fi < file.function_count(); ++fi)
        {
            for (auto& line : file.get_lines())
            {
                if (1 == line.fVals.at(fi))
                {
                    auto vars = cube_to_bool_vars(line.cube);
                    functions[fi].emplace_back(vars.empty() ? creator.just_val(1) : creator.product(std::begin(vars), std::end(vars)));
                }
            }
        }

        return pla_function {std::move(functions)};
    }

    inline pla_function::pla_function
        (function_v functions) : 
        functions_ {std::move(functions)}
    {
    }

    template<class BoolFunctionInput, class GetVarVal>
    inline auto pla_function::get_value
        (BoolFunctionInput const& input, std::size_t const i) const -> bool_t
    {
        return std::any_of(std::begin(functions_[i]), std::end(functions_[i]), [&input](auto const& f)
        {
            return f.get_value(input);
        });
    }
}

#endif