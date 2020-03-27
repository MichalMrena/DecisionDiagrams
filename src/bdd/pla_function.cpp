#include "pla_function.hpp"

#include <stdexcept>
#include "pla_file.hpp"
#include "bdd_creator.hpp"

namespace mix::dd
{
    auto pla_function::create_from_file
        (const pla_file& file) -> pla_function
    {
        using creator_t = bdd_creator<empty_t, empty_t>;
        creator_t creator;
        std::vector< std::vector<bdd_t> > functionsAsSops(file.function_count());

        for (int32_t fi {0}; fi < file.function_count(); ++fi)
        {
            for (auto& line : file.get_lines())
            {
                if (1 == line.fVals.at(fi))
                {
                    functionsAsSops.at(fi).emplace_back(creator.create_product(line.cube.begin(), line.cube.end(), 1));
                }
            }
        }

        return pla_function 
        {
            static_cast<index_t>(file.variable_count())
          , std::move(functionsAsSops)
        };
    }

    pla_function::pla_function( const index_t pVariableCount
                              , std::vector< std::vector<bdd_t> > functionsAsSops) :
        variableCount    {pVariableCount}
      , functionsAsSops_ {std::move(functionsAsSops)}
      , activeFunction_  {std::ref(functionsAsSops_.front())}
    {
    }

    auto pla_function::get_f_val
        (const input_bits_t& input) const -> bool_t
    {
        for (auto& productDiagram : activeFunction_.get())
        {
            if (1 == productDiagram.get_value(input))
            {
                return 1;
            }
        }

        return 0;
    }

    auto pla_function::get_f_val
        (const var_vals_t input) const -> bool_t
    {
        for (auto& productDiagram : activeFunction_.get())
        {
            if (1 == productDiagram.get_value(input))
            {
                return 1;
            }
        }

        return 0;
    }

    auto pla_function::get_f_val_r
        (const var_vals_t input) const -> bool_t
    {
        return this->get_f_val(reverse_vals(input, this->variable_count()));
    }

    auto pla_function::variable_count
        () const -> index_t
    {
        return this->variableCount;
    }

    auto pla_function::at
        (const index_t fIndex) -> pla_function&
    {
        activeFunction_ = std::ref(functionsAsSops_.at(fIndex));
        return *this;
    }

    auto get_f_val<pla_function>::operator()
        (const pla_function& in, const var_vals_t i) -> bool_t
    {
        return in.get_f_val(i);
    }

    auto get_f_val_r<pla_function>::operator()
        (const pla_function& in, const var_vals_t i) -> bool_t
    {
        return in.get_f_val_r(i);
    }

    auto var_count<pla_function>::operator()
        (const pla_function& in) -> index_t
    {
        return in.variable_count();
    }
}