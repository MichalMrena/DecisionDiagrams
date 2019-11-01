#ifndef MIX_DD_BOOL_FUNCTION
#define MIX_DD_BOOL_FUNCTION

#include <string>
#include <vector>
#include <sstream>

#include "typedefs.hpp"
#include "utils/io_exception.hpp"

namespace mix::dd
{
    class bool_function
    {
    private:
        std::vector<std::string> varNames;
        std::vector<log_val_t> values;

    public:
        using var_names_iterator = typename std::vector<std::string>::const_iterator;

        static auto load_from_file (const std::string& filePath) -> bool_function;

    public:
        bool_function(const bool_function& other);
        bool_function(bool_function&& other);

        // BEGIN must have interface
        auto operator[]     (const input_t input) const -> log_val_t;
        auto variable_count () const -> size_t;

        auto begin () const -> var_names_iterator;
        auto end   () const -> var_names_iterator;
        // END must have interface

        auto to_string (std::ostream& ostr) const -> void;

    private:
        static auto str_to_log_val (const std::string& str) -> log_val_t;

        template<class InputIt>
        static auto raw_vals_to_input (InputIt begin, InputIt end, size_t varsCount) -> input_t;

    private:
        bool_function(std::vector<std::string>&& pVarNames
                  , std::vector<log_val_t>&&  pValues);
    };

    template<class InputIt>
    auto bool_function::raw_vals_to_input (InputIt begin, InputIt end, size_t varsCount) -> input_t
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