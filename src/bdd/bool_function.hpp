#ifndef _MIX_DD_BDD_FUNCTION_INPUT_
#define _MIX_DD_BDD_FUNCTION_INPUT_

#include "../dd/typedefs.hpp"
#include "../utils/bits.hpp"

namespace mix::dd
{
    /**
        Returns value of the function for given variable values.
        Values of the variables are represented by bits of the number i.
        Lowest bit represents value of x(0).
    */
    template<class BoolFunction>
    struct get_f_val
    {
        auto operator() ( const BoolFunction& in
                        , const var_vals_t i) -> bool_t;
    };

    /**
        Returns value of the function for given variable values.
        Values of the variables are represented by bits of the number i.
        Lowest bit represents value of x(n).
    */
    template<class BoolFunction>
    struct get_f_val_r
    {
        auto operator() ( const BoolFunction& in
                        , const var_vals_t i) -> bool_t;
    };

    /**
        Returns number of variables of the function.
    */
    template<class BoolFunction>
    struct var_count
    {
        auto operator() (const BoolFunction& in) -> index_t;
    };

    /**
        Auxiliary function for reversing order of the bits.
        It is useful for the implementation of get_f_val and get_f_val_r.
    */
    inline auto reverse_vals ( const var_vals_t varVals
                             , const index_t varCount ) -> var_vals_t
    {
        const auto shift {(8 * sizeof(var_vals_t) - varCount)};
        return utils::reverse_bits(varVals) >> shift;
    }
}

#endif