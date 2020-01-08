#include "truth_table.hpp"

#include <fstream>
#include <sstream>
#include <utility>
#include <algorithm>
#include <cmath>
#include "utils/file_reader.hpp"
#include "utils/string_utils.hpp"
#include "utils/math_utils.hpp"
#include "utils/io_exception.hpp"

namespace mix::dd
{
    auto truth_table::load_from_file 
        (const std::string& filePath) -> truth_table
    {
        using x_y_pair = std::pair<input_t, log_val_t>;
        
        utils::file_reader reader {filePath};
        reader.throw_if_cant_read();

        std::string line;
        reader.read_line_except(line); // comment with function
        reader.read_line_except(line);

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
            reader.read_line_except(line);
            const auto tokens {utils::to_words(line)};

            const log_val_t functionValue {truth_table::str_to_log_val(tokens.back())};

            // TODO treba sa rozhodnúť či tu toto vôbec bude
            functionValues.push_back(std::make_pair(
                row_vals_to_input(tokens, varNames.size())
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

        return truth_table {std::move(values)};
    }

    truth_table::truth_table(const truth_table& other) :
        values {other.values}
    {
    }

    truth_table::truth_table(truth_table&& other) :
        values {std::move(other.values)}
    {
    }

    auto truth_table::operator[] 
        (const input_t input) const -> log_val_t
    {
        return this->values[input];
    }

    auto truth_table::variable_count 
        () const -> size_t
    {
        return static_cast<size_t>(std::log2(this->values.size()));
    }

    auto truth_table::str_to_log_val 
        (const std::string& str) -> log_val_t
    {
        if ("0" == str) return 0;
        if ("1" == str) return 1;

        throw utils::io_exception("Unexpected function value: " + str);
    }

    auto truth_table::row_vals_to_input 
        (const std::vector<std::string>& rowVals, size_t varsCount) -> input_t
    {
        input_t input {0};
        size_t varsProcessed {0};

        for (size_t i {0}; i < rowVals.size();)
        {
            std::string strVal {rowVals[i]};
            if ("0" != strVal && "1" != strVal)
            {
                throw utils::io_exception {"Invalid variable value."};
            }

            log_val_t logVal {"0" == strVal ? (int8_t)0 : (int8_t)1};

            input |= logVal;
            ++i;
            ++varsProcessed;

            if (i == varsCount) break;

            input <<= 1;
        }

        if (varsProcessed < varsCount)
        {
            throw utils::io_exception {"Unexpected end of line."};
        }

        if (varsProcessed > varsCount)
        {
            throw utils::io_exception {"Too many variable values."};
        }
        
        return input;
    }

    truth_table::truth_table(std::vector<log_val_t>&& pValues) :
        values {pValues}
    {
    }
}