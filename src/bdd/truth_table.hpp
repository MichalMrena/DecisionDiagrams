#ifndef _MIX_DD_TRUTH_TABLE_
#define _MIX_DD_TRUTH_TABLE_

#include <string>
#include <vector>
#include "bool_function.hpp"

namespace mix::dd
{
    class truth_table
    {
    private:
        std::vector<bool> valuesR;

    public:
        static auto load_from_file (const std::string& filePath) -> truth_table;

    public:
        truth_table(const truth_table& other);
        truth_table(truth_table&& other);

        auto get_f_val      (const var_vals_t input) const -> bool_t; 
        auto get_f_val_r    (const var_vals_t input) const -> bool_t; 
        auto variable_count () const -> index_t;

    private:
        truth_table(std::vector<bool> pValuesR);
    };

    template<>
    struct get_f_val<truth_table>
    {
        auto operator() ( const truth_table& in
                        , const var_vals_t i) -> bool_t;
    };

    template<>
    struct get_f_val_r<truth_table>
    {
        auto operator() ( const truth_table& in
                        , const var_vals_t i) -> bool_t;
    };

    template<>
    struct var_count<truth_table>
    {
        auto operator() (const truth_table& in) -> index_t;
    };
}

#endif