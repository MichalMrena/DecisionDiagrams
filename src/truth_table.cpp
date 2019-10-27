#include "truth_table.hpp"

#include <fstream>
#include <utility>
#include <algorithm>
#include "utils/file_reader.hpp"
#include "utils/string_utils.hpp"
#include "utils/math_utils.hpp"
#include "utils/io_exception.hpp"

namespace mix::dd
{
    auto truth_table::load_from_file (const std::string& filePath) -> truth_table
    {
        using x_y_pair = std::pair<input_t, log_val_t>;
        
        utils::file_reader reader {filePath};
        reader.throw_if_cant_read();

        std::string line;
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

            const log_val_t functionValue {truth_table::str_to_log_val(tokens.back())};

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

        return truth_table {std::move(varNames), std::move(values)};
    }

    truth_table::truth_table(const truth_table& other) :
        varNames {other.varNames}
      , values   {other.values}
    {
    }

    truth_table::truth_table(truth_table&& other) :
        varNames {std::move(other.varNames)}
      , values   {std::move(other.values)}
    {
    }

    auto truth_table::operator[] (const input_t input) const -> log_val_t
    {
        return this->values[input];
    }

    auto truth_table::to_string(std::ostream& ostr) const -> void
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

    auto truth_table::begin () const -> var_names_iterator
    {
        return this->varNames.cbegin();
    }

    auto truth_table::end () const -> var_names_iterator
    {
        return this->varNames.cend();
    }

    auto truth_table::str_to_log_val (const std::string& str) -> log_val_t
    {
        if ("0" == str) return 0;
        if ("1" == str) return 1;

        throw utils::io_exception("Unexpected function value: " + str);
    }

    truth_table::truth_table(std::vector<std::string>&& pVarNames
                           , std::vector<log_val_t>&&  pValues) :
        varNames {pVarNames}
      , values   {pValues}
    {
    }
}