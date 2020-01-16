#ifndef _MIX_DD_LAMBDA_BOOL_F_
#define _MIX_DD_LAMBDA_BOOL_F_

#include <bitset>
#include <functional>
#include "bool_function.hpp"
#include "../dd/typedefs.hpp"

namespace mix::dd
{
    class var_vals
    {
    private:
        const var_vals_t values;

    public:
        var_vals(const var_vals_t pValues);
        auto operator() (const index_t i) const -> bool_t;
    };

    class lambda_bool_f
    {
    public:
        using lambda_t = std::function<bool_t(var_vals)>;

    private:
        lambda_t lambda;
        index_t  variableCount;

    public:
        lambda_bool_f( const index_t pVariableCount
                     , lambda_t pLambda);

        auto get_f_val      (const var_vals_t input) const -> bool_t;
        auto get_f_val_r    (const var_vals_t input) const -> bool_t;
        auto variable_count () const -> index_t;
    };

    template<>
    struct get_f_val<lambda_bool_f>
    {
        auto operator() ( const lambda_bool_f& in
                        , const var_vals_t i) -> bool_t;
    };

    template<>
    struct get_f_val_r<lambda_bool_f>
    {
        auto operator() ( const lambda_bool_f& in
                        , const var_vals_t i) -> bool_t;
    };

    template<>
    struct var_count<lambda_bool_f>
    {
        auto operator() (const lambda_bool_f& in) -> index_t;
    };    
}

#endif