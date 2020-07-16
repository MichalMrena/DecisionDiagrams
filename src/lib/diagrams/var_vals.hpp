#ifndef MIX_DD_VAR_VALS_
#define MIX_DD_VAR_VALS_

#include "typedefs.hpp"
#include "../utils/more_type_traits.hpp"

#include <vector>
#include <bitset>
#include <array>
#include <type_traits>

namespace mix::dd
{
    namespace aux_impl
    {
        template<class T>
        using int_type = std::enable_if_t< std::is_integral_v<T> 
                                        && !std::is_same_v<bool, T> >;

        template<class T>
        using vector_type = std::enable_if_t< utils::is_std_vector_v<T>
                                           && std::is_integral_v<typename T::value_type> >;

        template<class T>
        using array_type = std::enable_if_t< utils::is_std_array_v<T>
                                          && std::is_integral_v<typename T::value_type> >;                                            

        template<class T>
        using bitset_type = std::enable_if_t< utils::is_std_bitset_v<T> >;

        template<std::size_t P>
        using log_t = typename log_val_traits<P>::type;
    }

    /**
        Functor that extract value of i-th variable from a container of variables.
        This general class has no definition. Definitions are provided
        via template specialization and SFINE below.
        see. https://cpppatterns.com/patterns/class-template-sfinae.html
     */
    template<std::size_t P, class VarVals, class = void>
    struct get_var_val;

    /**
        Specialized definition for integral types except bool.
     */
    template<std::size_t P, class VarVals>
    struct get_var_val<P, VarVals, aux_impl::int_type<VarVals>>
    {
        auto operator() (VarVals const& in, index_t const i) const -> aux_impl::log_t<P>
        {
            if (i >= sizeof(VarVals))
            {
                throw std::out_of_range {"Bit index out of range."};
            }
            return (in >> i) & 1;
        }
    };

    /**
        Specialized definition for std::vector of integral type.
     */
    template<std::size_t P, class VarVals>
    struct get_var_val<P, VarVals, aux_impl::vector_type<VarVals>>
    {
        auto operator() (VarVals const& in, index_t const i) const -> aux_impl::log_t<P>
        {
            return in.at(i);
        }
    };

    /**
        Specialized definition for std::array of integral type.
     */
    template<std::size_t P, class VarVals>
    struct get_var_val<P, VarVals, aux_impl::array_type<VarVals>>
    {
        auto operator() (VarVals const& in, index_t const i) const -> aux_impl::log_t<P>
        {
            return in.at(i);
        }
    };

    /**
        Specialized definition for std::bitset.
     */
    template<std::size_t P, class VarVals>
    struct get_var_val<P, VarVals, aux_impl::bitset_type<VarVals>>
    {
        auto operator() (VarVals const& in, index_t const i) const -> aux_impl::log_t<P>
        {
            return in.test(i);
        }
    };


    /**
        Functor that sets value of i-th variable in a container of variables.
        This general class has no definition. Definitions are provided
        via template specialization and SFINE below.

        For now only two valued logic is supported.
     */
    template<class VarVals, class = void>
    struct set_var_val;

    /**
        Specialized definition for std::bitset.
     */
    template<class VarVals> 
    struct set_var_val<VarVals, aux_impl::bitset_type<VarVals>>
    {
        auto operator() (VarVals& vars, index_t const i, bool const v) const -> void
        {
            vars.set(i, v);
        }
    };

    /**
        Specialized definition for integral types except bool.
     */
    template<class VarVals> 
    struct set_var_val<VarVals, aux_impl::int_type<VarVals>>
    {
        auto operator() (VarVals& vars, index_t const i, bool const v) const -> void
        {
            if (i >= sizeof(VarVals))
            {
                throw std::out_of_range {"Bit index out of range."};
            }
            vars |= 1 << i;
        }
    };
}

#endif