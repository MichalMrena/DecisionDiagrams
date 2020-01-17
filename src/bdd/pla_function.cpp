#include "pla_function.hpp"

#include <stdexcept>
#include "pla_file.hpp"

namespace mix::dd
{
    auto pla_function::create_from_file
        (const pla_file& file) -> pla_function
    {
        const auto& lines {file.get_lines()};

        std::vector< std::vector<bdd_t> > lineDiagrams(file.line_count());
        for (auto& subVector : lineDiagrams)
        {
            subVector.reserve(file.function_count());
        }

        creator_t creator;
        size_t currentLine {0};

        for (const auto& line : lines)
        {
            for (const auto fVal : line.fVals)
            {
                lineDiagrams[currentLine].emplace_back(
                    creator.create_product(line.varVals, fVal)
                );
            }

            ++currentLine;
        }

        return pla_function 
        {
            static_cast<index_t>(file.variable_count())
          , std::move(lineDiagrams)
        };
    }

    pla_function::pla_function( const index_t pVariableCount
                              , std::vector< std::vector<bdd_t> > pLines) :
        variableCount {pVariableCount}
      , activeOutput  {0}
      , lines         {std::move(pLines)}
    {
    }

    auto pla_function::get_f_val
        (const var_vals_t input) const -> bool_t
    {
        for (const auto& line : this->lines)
        {
            if (1 == line.at(this->activeOutput).get_value(input))
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
        if (fIndex >= this->lines.at(0).size())
        {
            throw std::invalid_argument {"Function index out of bounds."};
        }

        this->activeOutput = fIndex;

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