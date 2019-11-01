#include "bool_function.hpp"

#include <fstream>
#include <utility>
#include <algorithm>
#include "utils/file_reader.hpp"
#include "utils/string_utils.hpp"
#include "utils/math_utils.hpp"
#include "utils/io_exception.hpp"

namespace mix::dd
{
    auto bool_function::load_from_file (const std::string& filePath) -> bool_function
    {
        using x_y_pair = std::pair<input_t, log_val_t>;
        
        utils::file_reader reader {filePath};
        reader.throw_if_cant_read();

        std::string line;
        reader.next_line_except(line); // comment with function
        reader.next_line_except(line);

        auto varNames {utils::to_words(line)};

        if (varNames.size() > 63)
        {
            throw utils::io_exception("Too many variables.");
        }

        const auto lineCount {utils::pow(static_cast<size_t>(2), varNames.size())};

        std::vector<x_y_pair> functionValues;
        functionValues.reserve(lineCount);

        for (size_t i {0}; i < lineCount; ++i)
        {
            reader.next_line_except(line);
            const auto tokens {utils::to_words(line)};

            const log_val_t functionValue {bool_function::str_to_log_val(tokens.back())};

            // TODO treba sa rozhodnúť či tu toto vôbec bude
            functionValues.push_back(std::make_pair(
                raw_vals_to_input(tokens.begin(), tokens.end() - 1, varNames.size())
              , functionValue
            ));
        }
        
        // TODO treba sa rozhodnúť či tu toto vôbec bude
        std::sort(functionValues.begin(), functionValues.end(), [](const x_y_pair& p1, const x_y_pair& p2){
            return p1.first < p2.first;
        });

        std::vector<log_val_t> values;
        values.reserve(functionValues.size());

        for (x_y_pair& pair : functionValues)
        {
            values.push_back(pair.second);
        }

        return bool_function {std::move(varNames), std::move(values)};
    }

    bool_function::bool_function(const bool_function& other) :
        varNames {other.varNames}
      , values   {other.values}
    {
    }

    bool_function::bool_function(bool_function&& other) :
        varNames {std::move(other.varNames)}
      , values   {std::move(other.values)}
    {
    }

    auto bool_function::operator[] (const input_t input) const -> log_val_t
    {
        return this->values[input];
    }

    auto bool_function::variable_count () const -> size_t
    {
        return this->varNames.size();
    }

    auto bool_function::to_string(std::ostream& ostr) const -> void
    {
        ostr << "Variables: " << '\n';
        for (auto& var : this->varNames)
        {
            ostr << var << ' ';
        }
        ostr << '\n';

        ostr << "Values: " << '\n';
        for (auto& val : this->values)
        {
            ostr << static_cast<int>(val) << ' ';
        }
        ostr << '\n';
    }

    auto bool_function::begin () const -> var_names_iterator
    {
        return this->varNames.cbegin();
    }

    auto bool_function::end () const -> var_names_iterator
    {
        return this->varNames.cend();
    }

    auto bool_function::str_to_log_val (const std::string& str) -> log_val_t
    {
        if ("0" == str) return 0;
        if ("1" == str) return 1;

        throw utils::io_exception("Unexpected function value: " + str);
    }

    bool_function::bool_function(std::vector<std::string>&& pVarNames
                           , std::vector<log_val_t>&&  pValues) :
        varNames {pVarNames}
      , values   {pValues}
    {
    }
}