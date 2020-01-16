#ifndef _MIX_DD_BDD_FUNCTION_INPUT_
#define _MIX_DD_BDD_FUNCTION_INPUT_

#include "../dd/typedefs.hpp"
#include "../utils/bits.hpp"

namespace mix::dd
{
    template<class BoolFunction>
    struct get_f_val
    {
        auto operator() ( const BoolFunction& in
                        , const var_vals_t i) -> bool_t;
    };

    template<class BoolFunction>
    struct get_f_val_r
    {
        auto operator() ( const BoolFunction& in
                        , const var_vals_t i) -> bool_t;
    };

    template<class BoolFunction>
    struct var_count
    {
        auto operator() (const BoolFunction& in) -> index_t;
    };

    inline auto reverse_vals ( const var_vals_t varVals
                             , const index_t varCount) -> var_vals_t
    {
        const auto shift {(8 * sizeof(var_vals_t) - varCount)};
        return utils::reverse_bits(varVals) >> shift;
    }
}

#endif