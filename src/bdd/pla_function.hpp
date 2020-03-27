#ifndef _MIX_DD_PLA_FUNCTION_
#define _MIX_DD_PLA_FUNCTION_

#include <vector>
#include <functional>
#include <bitset>
#include "bdd.hpp"
#include "bool_function.hpp"

namespace mix::dd
{
    class pla_file;

    class pla_function
    {
    public:
        using bdd_t        = bdd<empty_t, empty_t>;
        using input_bits_t = std::bitset<128>;

    private:
        index_t variableCount;
        std::vector< std::vector<bdd_t> > functionsAsSops_;
        std::reference_wrapper< std::vector<bdd_t> > activeFunction_;

    public:
        static auto create_from_file (const pla_file& file) -> pla_function;

    public:
        auto get_f_val      (const input_bits_t& input) const -> bool_t; 
        auto get_f_val      (const var_vals_t input)    const -> bool_t; 
        auto get_f_val_r    (const var_vals_t input)    const -> bool_t; 
        auto at             (const index_t fIndex)            -> pla_function&;
        auto variable_count () const -> index_t;

    private:
        pla_function( const index_t pVariableCount
                    , std::vector< std::vector<bdd_t> > functionsAsSops );
    };

    template<>
    struct get_f_val<pla_function>
    {
        auto operator() ( const pla_function& in
                        , const var_vals_t i ) -> bool_t;
    };

    template<>
    struct get_f_val_r<pla_function>
    {
        auto operator() ( const pla_function& in
                        , const var_vals_t i ) -> bool_t;
    };

    template<>
    struct var_count<pla_function>
    {
        auto operator() (const pla_function& in) -> index_t;
    };
}

#endif