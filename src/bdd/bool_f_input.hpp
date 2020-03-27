#ifndef _MIX_DD_BOOL_F_INPUT_
#define _MIX_DD_BOOL_F_INPUT_

#include <vector>
#include <bitset>
#include "../data_structures/bit_vector.hpp"
#include "../dd/typedefs.hpp"

namespace mix::dd
{
    template<class VariableValues>
    struct get_var_val
    {
        auto operator() (const VariableValues& in, const index_t i) const -> bool_t;
    };

    template<>
    struct get_var_val< std::vector<bool> >
    {
        auto operator() (const std::vector<bool>& in, const index_t i) const -> bool_t
        {
            return in[i];
        }
    };
    
    template<>
    struct get_var_val< var_vals_t >
    {
        auto operator() (const var_vals_t in, const index_t i) const -> bool_t
        {
            return (in >> i) & 1;
        }
    };
    
    template<size_t N>
    struct get_var_val< std::bitset<N> >
    {
        auto operator() (const std::bitset<N>& in, const index_t i) const -> bool_t
        {
            return in.test(i);
        }
    };
    
    template<size_t N>
    struct get_var_val< std::array<bool_t, N> >
    {
        auto operator() (const std::array<bool_t, N>& in, const index_t i) const -> bool_t
        {
            return in[i];
        }
    };
    
    template<size_t N>
    struct get_var_val< bit_vector<N, bool_t> >
    {
        auto operator() (const bit_vector<N, bool_t>& in, const index_t i) const -> bool_t
        {
            return in.at(i);
        }
    };


    template<class VariableValues>
    struct set_var_val
    {
        auto operator() ( VariableValues& vars
                        , const index_t i
                        , const bool_t  val ) const -> void;
    };

    template<size_t N> 
    struct set_var_val<std::bitset<N>>
    {
        auto operator() ( std::bitset<N>& vars
                        , const index_t   i
                        , const bool_t    val ) const -> void
        {
            vars[i] = val;
        }
    };
}

#endif