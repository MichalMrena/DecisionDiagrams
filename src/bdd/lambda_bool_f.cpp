#include "lambda_bool_f.hpp"

namespace mix::dd
{
    var_vals::var_vals(const var_vals_t pValues) :
        values {pValues}
    {
    }

    auto var_vals::operator()
        (const index_t i) const -> bool_t
    {
        return (this->values >> i) & 1;
    }


    lambda_bool_f::lambda_bool_f( const index_t pVariableCount
                                , lambda_t pLambda) :
        lambda        {pLambda}
      , variableCount {pVariableCount}
    {
    }

    auto lambda_bool_f::get_f_val 
        (const var_vals_t input) const -> bool_t
    {
        return this->lambda(input);
    }

    auto lambda_bool_f::get_f_val_r
        (const var_vals_t input) const -> bool_t
    {
        return this->lambda(reverse_vals(input, this->variable_count()));
    }


    auto lambda_bool_f::variable_count 
        () const -> index_t
    {
        return this->variableCount;
    }

    auto get_f_val<lambda_bool_f>::operator()
        (const lambda_bool_f& in, const var_vals_t i) -> bool_t
    {
        return in.get_f_val(i);
    }

    auto get_f_val_r<lambda_bool_f>::operator()
        (const lambda_bool_f& in, const var_vals_t i) -> bool_t
    {
        return in.get_f_val_r(i);
    }

    auto var_count<lambda_bool_f>::operator()
        (const lambda_bool_f& in) -> index_t
    {
        return in.variable_count();
    }
}