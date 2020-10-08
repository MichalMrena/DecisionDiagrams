#ifndef MIX_DD_OPERATORS_STATIC_CHECK_HPP
#define MIX_DD_OPERATORS_STATIC_CHECK_HPP

#include "operators.hpp"

#include <type_traits>

namespace mix::dd
{
    /**
        Base case that catches everything that is not specialized.
        This means that if a custom user provided operation is used
        this check is true and it is up to the user to ensure correct
        behaviour of the operation.
    */
    template<std::size_t, class>
    struct check_op : public std::true_type {};

    /**
        This specialization catches all operations defined in `operators.hpp`.
    */
    template<std::size_t P1, template<std::size_t, domain_e> class Op, std::size_t P2, domain_e Domain>
    struct check_op<P1, Op<P2, Domain>> : public std::bool_constant<P1 == P2> {};

    /**
        Helper constant as usual for easier use.
     */
    template<std::size_t P, class Op>
    inline constexpr auto check_op_v = check_op<P, Op>::value;
}

#endif