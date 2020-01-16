#include "truth_table.hpp"

#include <fstream>
#include <sstream>
#include <utility>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <bitset>
#include "../utils/file_reader.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/math_utils.hpp"

namespace mix::dd
{
    namespace
    {
        auto chars_to_bits (std::string s) -> var_vals_t
        {
            return std::bitset<64> {s}.to_ullong();
        }
    }

    auto truth_table::load_from_file 
        (const std::string& filePath) -> truth_table
    {
        using line_pair = std::pair<var_vals_t, bool_t>;
        
        utils::file_reader reader {filePath};
        reader.throw_if_cant_read();

        const auto firstLine {utils::to_head_tail(reader.peek_line_except())};
        const auto lineCount {utils::two_pow(firstLine.first.size())};

        std::vector<line_pair> functionValues;
        functionValues.reserve(lineCount);

        for (size_t i {0}; i < lineCount; ++i)
        {
            const auto linePair {utils::to_head_tail(utils::trim(reader.read_line_except()))};
            const auto varVals  {chars_to_bits(std::move(linePair.first))};
            const auto fVal     {linePair.second == "1"};

            functionValues.emplace_back(varVals, fVal);
        }
        
        std::sort( functionValues.begin(), functionValues.end() );

        std::vector<bool> values;
        values.reserve(functionValues.size());

        for (auto& [varVals, fVal] : functionValues)
        {
            values.push_back(fVal);
        }

        return truth_table {std::move(values)};
    }

    truth_table::truth_table(const truth_table& other) :
        valuesR {other.valuesR}
    {
    }

    truth_table::truth_table(truth_table&& other) :
        valuesR {std::move(other.valuesR)}
    {
    }

    auto truth_table::get_f_val
        (const var_vals_t input) const -> bool_t
    {
        return this->valuesR[reverse_vals(input, this->variable_count())];
    }

    auto truth_table::get_f_val_r
        (const var_vals_t input) const -> bool_t
    {
        return this->valuesR[input];
    }

    auto truth_table::variable_count 
        () const -> index_t
    {
        return static_cast<index_t>(std::log2(this->valuesR.size()));
    }

    truth_table::truth_table(std::vector<bool> pValuesR) :
        valuesR {pValuesR}
    {
    }

    auto get_f_val<truth_table>::operator()
        (const truth_table& in, const var_vals_t i) -> bool_t
    {
        return in.get_f_val(i);
    }

    auto get_f_val_r<truth_table>::operator()
        (const truth_table& in, const var_vals_t i) -> bool_t
    {
        return in.get_f_val_r(i);
    }

    auto var_count<truth_table>::operator()
        (const truth_table& in) -> index_t
    {
        return in.variable_count();
    }
}