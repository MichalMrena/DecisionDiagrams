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
        using x_y_pair   = std::pair<input_t, log_val_t>;
        using x_y_pair_v = std::vector<x_y_pair>;
        
        utils::file_reader reader {filePath};
        reader.throw_if_cant_read();

        std::string line;
        reader.next_line_except(line);

        auto varNames  {utils::to_words(line)};
        auto lineCount {utils::pow(static_cast<size_t>(2), varNames.size())};

        x_y_pair_v functionValues;
        functionValues.reserve(lineCount);

        for (size_t i {0}; i < lineCount; ++i)
        {
            reader.next_line_except(line);
            auto tokens {utils::to_words(line)};

            log_val_t functionValue {truth_table::str_to_log_val(tokens.back())};

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

        for (auto& pair : functionValues)
        {
            values.push_back(pair.second);
        }

        return truth_table {std::move(varNames), std::move(values)};
    }

    auto truth_table::to_string(std::ostream& ostr) -> void
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