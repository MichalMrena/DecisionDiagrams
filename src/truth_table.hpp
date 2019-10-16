#ifndef MIX_DD_TRUTH_TABLE
#define MIX_DD_TRUTH_TABLE

#include <string>
#include <vector>
#include <sstream>

#include "typedefs.hpp"
#include "utils/io_exception.hpp"

namespace mix::dd
{
    class truth_table
    {
    private:
        std::vector<std::string> varNames;
        std::vector<log_val_t> values;

    public:
        static auto load_from_file (const std::string& filePath) -> truth_table;

    public:
        auto get_function_value (input_t input) -> log_val_t;
        auto to_string (std::ostream& ostr) -> void;

    private:
        static auto str_to_log_val (const std::string& str) -> log_val_t;

        template<class InputIt>
        static auto raw_vals_to_input (InputIt begin, InputIt end, size_t varsCount) -> input_t;

    private:
        truth_table(std::vector<std::string>&& pVarNames
                  , std::vector<log_val_t>&&  pValues);
    };

    template<class InputIt>
    auto truth_table::raw_vals_to_input (InputIt begin, InputIt end, size_t varsCount) -> input_t
    {
        std::ostringstream ostr;

        size_t varsProcessed {0};
        while (begin != end)
        {
            ostr << *begin;

            ++varsProcessed;
            ++begin;
        }

        if (varsProcessed < varsCount)
        {
            throw utils::io_exception {"Unexpected end of line."};
        }

        if (varsProcessed > varsCount)
        {
            throw utils::io_exception {"Too many variable values."};
        }
        
        size_t idx;
        auto inputStr {ostr.str()};
        auto inputVal {std::stoull(inputStr, &idx, 2)};

        if (idx != inputStr.size())
        {
            throw utils::io_exception {"Invalid variable value."};
        }

        return inputVal;
    }
    
}

#endif