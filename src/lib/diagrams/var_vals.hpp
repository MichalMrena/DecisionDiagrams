#ifndef MIX_DD_VAR_VALS_HPP
#define MIX_DD_VAR_VALS_HPP

#include "typedefs.hpp"
#include "../utils/more_type_traits.hpp"

namespace mix::dd
{
    namespace vv_impl
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
        Function object that extract value of i-th variable from a container of variables.
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
    struct get_var_val<P, VarVals, vv_impl::int_type<VarVals>>
    {
        static_assert(2 == P);

        [[nodiscard]] constexpr auto operator()
            (VarVals const& in, index_t const i) const -> vv_impl::log_t<P>
        {
            return (in >> i) & 1;
        }
    };

    /**
        Specialized definition for `std::vector` of integral type.
     */
    template<std::size_t P, class VarVals>
    struct get_var_val<P, VarVals, vv_impl::vector_type<VarVals>>
    {
        [[nodiscard]] auto operator()
            (VarVals const& in, index_t const i) const -> vv_impl::log_t<P>
        {
            return in[i];
        }
    };

    /**
        Specialized definition for `std::array` of integral type.
     */
    template<std::size_t P, class VarVals>
    struct get_var_val<P, VarVals, vv_impl::array_type<VarVals>>
    {
        [[nodiscard]] constexpr auto operator()
            (VarVals const& in, index_t const i) const -> vv_impl::log_t<P>
        {
            return in[i];
        }
    };

    /**
        Specialized definition for `std::bitset`.
     */
    template<std::size_t P, class VarVals>
    struct get_var_val<P, VarVals, vv_impl::bitset_type<VarVals>>
    {
        static_assert(2 == P);

        [[nodiscard]] constexpr auto operator()
            (VarVals const& in, index_t const i) const -> bool_t
        {
            return in[i];
        }
    };


    /**
        Function object that sets value of i-th variable in a container of variables.
        This general class has no definition. Definitions are provided
        via template specialization and SFINE below.
     */
    template<std::size_t P, class VarVals, class = void>
    struct set_var_val;

    /**
        Specialized definition for `std::bitset`.
     */
    template<std::size_t P, class VarVals>
    struct set_var_val<P, VarVals, vv_impl::bitset_type<VarVals>>
    {
        static_assert(2 == P);

        auto operator()
            (VarVals& vars, index_t const i, bool_t const v) const -> void
        {
            vars.set(i, v);
        }
    };

    /**
        Specialized definition for integral types except bool.
     */
    template<std::size_t P, class VarVals>
    struct set_var_val<P, VarVals, vv_impl::int_type<VarVals>>
    {
        static_assert(2 == P);

        constexpr auto operator()
            (VarVals& vars, index_t const i, bool_t const v) const -> void
        {
            vars = (vars & ~(1ul << i)) | (v << i);
        }
    };

    /**
        Specialized definition for `std::array`.
     */
    template<std::size_t P, class VarVals>
    struct set_var_val<P, VarVals, vv_impl::array_type<VarVals>>
    {
        using log_t = typename log_val_traits<P>::type;

        constexpr auto operator()
            (VarVals& vars, index_t const i, log_t const v) const -> void
        {
            vars[i] = v;
        }
    };
}

#endif