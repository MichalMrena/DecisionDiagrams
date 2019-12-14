#ifndef _MIX_DD_TRUTH_TABLE_
#define _MIX_DD_TRUTH_TABLE_

#include <string>
#include <vector>
#include "bool_function.hpp"

namespace mix::dd
{
    class truth_table : public bool_function
    {
    private:
        std::vector<log_val_t> values;

    public:
        static auto load_from_file (const std::string& filePath) -> truth_table;

    public:
        truth_table(const truth_table& other);
        truth_table(truth_table&& other);
        virtual ~truth_table() = default;

        auto operator[] (const input_t input) const -> log_val_t override;
        
        auto variable_count () const -> size_t override;

    private:
        static auto str_to_log_val (const std::string& str) -> log_val_t;

        static auto row_vals_to_input (const std::vector<std::string>& rowVals
                                     , size_t varsCount) -> input_t;

    private:
        truth_table(std::vector<log_val_t>&& pValues);
    };    
}

#endif