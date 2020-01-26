#ifndef _MIX_DD_PLA_FUNCTION_
#define _MIX_DD_PLA_FUNCTION_

#include <vector>
#include "bdd.hpp"
#include "bdd_creator.hpp"

namespace mix::dd
{
    class pla_file;

    class pla_function
    {
    private:
        using bdd_t     = bdd<empty_t, empty_t>;
        using creator_t = bdd_creator<empty_t, empty_t>;

    private:
        index_t variableCount;
        index_t activeOutput;
        std::vector< std::vector<bdd_t> > lines;

    public:
        static auto create_from_file (const pla_file& file) -> pla_function;

    public:
        auto get_f_val      (const var_vals_t input) const -> bool_t; 
        auto get_f_val_r    (const var_vals_t input) const -> bool_t; 
        auto variable_count () const -> index_t;

        auto at             (const index_t fIndex) -> pla_function&;

    private:
        pla_function( const index_t pVariableCount
                    , std::vector< std::vector<bdd_t> > pLines);
    };

    template<>
    struct get_f_val<pla_function>
    {
        auto operator() ( const pla_function& in
                        , const var_vals_t i) -> bool_t;
    };

    template<>
    struct get_f_val_r<pla_function>
    {
        auto operator() ( const pla_function& in
                        , const var_vals_t i) -> bool_t;
    };

    template<>
    struct var_count<pla_function>
    {
        auto operator() (const pla_function& in) -> index_t;
    };
}

#endif