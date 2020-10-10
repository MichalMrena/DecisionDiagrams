#ifndef MIX_DD_OPERATORS_HPP
#define MIX_DD_OPERATORS_HPP

#include "typedefs.hpp"
#include "../utils/more_functional.hpp"

#include <algorithm>
#include <functional>

namespace mix::dd
{
    namespace impl
    {
        struct pi_conj_t
        {
            [[nodiscard]]
            constexpr auto operator() 
                (bool_t const lhs, bool_t const rhs) const noexcept -> bool_t
            {
                auto constexpr U = log_val_traits<2>::undefined;
                return std::min({lhs, rhs, U});
            }
        };

        template<std::size_t P>
        using type = typename log_val_traits<P>::type;
    }

    enum class domain_e { homogenous, nonhomogenous };

    /**
        Wrapper for binary operations that are used in the `apply`.
        Handles these common cases for all operations in this order:
            - When we work with non-homogenous function it returns
              `nodomain` which means `apply` will create terminal vertex.
            - When one of the values is @p AbsorbingVal for given operation
              it returns @p AbsorbingVal which means `apply` will create
              terminal vertex.
            - When one of the values is from an internal node (nondetermined)
              it returns `nondetermined` which means apply will create
              internal node.
            - If none of the above is true it calls @p BinOp with given arguments
              which means apply will create terminal node.

        Common operations are defined below so
        you don't need to use this wrapper directly.

        @tparam BinOp stateless binary operation.
        @tparam P domain of the function.
        @tparam Domain specifies whether @p lhs or @p rhs can be `nodomain`.
        @tparam AbsorbingVal the value if @p BinOp has it or `undefined`.
     */
    template< class         BinOp
            , std::size_t   P
            , domain_e      Domain
            , impl::type<P> AbsorbingVal >
    struct bin_op
    {
        using log_t = typename log_val_traits<P>::type;

        [[nodiscard]]
        constexpr auto operator()
            (log_t const lhs, log_t const rhs) const noexcept (BinOp () (log_t{}, log_t{})) -> log_t
        {
            if constexpr (Domain == domain_e::nonhomogenous)
            {
                if (is_nodomain<P>(lhs) || is_nodomain<P>(rhs))
                {
                    return log_val_traits<P>::nodomain;
                }
            }

            if constexpr (!is_undefined<P>(AbsorbingVal))
            {
                if (AbsorbingVal == lhs || AbsorbingVal == rhs)
                {
                    return AbsorbingVal;
                }
            }

            if (is_nondetermined<P>(lhs) || is_nondetermined<P>(rhs))
            {
                return log_val_traits<P>::nondetermined;
            }

            return BinOp () (lhs, rhs);
        }
    };

    template<std::size_t P = 2, domain_e Domain = domain_e::homogenous>
    struct AND : public bin_op<std::logical_and<>, P, Domain, 0> {};

    template<std::size_t P = 2, domain_e Domain = domain_e::homogenous>
    struct OR : public bin_op<std::logical_or<>, P, Domain, 1> {};

    template<std::size_t P = 2, domain_e Domain = domain_e::homogenous>
    struct XOR : public bin_op<std::not_equal_to<>, P, Domain, log_val_traits<2>::undefined> {};

    template<std::size_t P = 2, domain_e Domain = domain_e::homogenous>
    struct PI_CONJ : public bin_op<impl::pi_conj_t, P, Domain, 0> {};

    template<std::size_t P = 2, domain_e Domain = domain_e::homogenous>
    struct NAND : public bin_op<utils::logical_negate<std::logical_and<>>, P, Domain, 0> {};

    template<std::size_t P = 2, domain_e Domain = domain_e::homogenous>
    struct NOR : public bin_op<utils::logical_negate<std::logical_or<>>, P, Domain, 1> {};

    template<std::size_t P, domain_e Domain = domain_e::nonhomogenous>
    struct EQUAL_TO : public bin_op<std::equal_to<>, P, Domain, log_val_traits<P>::undefined> {};

    template<std::size_t P, domain_e Domain = domain_e::nonhomogenous>
    struct LESS : public bin_op<std::less<>, P, Domain, log_val_traits<P>::undefined> {};

    template<std::size_t P, domain_e Domain = domain_e::nonhomogenous>
    struct LESS_EQUAL : public bin_op<std::less_equal<>, P, Domain, log_val_traits<P>::undefined> {};

    template<std::size_t P, domain_e Domain = domain_e::nonhomogenous>
    struct GREATER : public bin_op<std::greater<>, P, Domain, log_val_traits<P>::undefined> {};

    template<std::size_t P, domain_e Domain = domain_e::nonhomogenous>
    struct GREATER_EQUAL : public bin_op<std::greater_equal<>, P, Domain, log_val_traits<P>::undefined> {};

    template<std::size_t P, domain_e Domain = domain_e::nonhomogenous>
    struct MIN : public bin_op<utils::min, P, Domain, 0> {};

    template<std::size_t P, domain_e Domain = domain_e::nonhomogenous>
    struct MAX : public bin_op<utils::max, P, Domain, P - 1> {};

    template<std::size_t P, domain_e Domain = domain_e::nonhomogenous>
    struct PLUS_MOD : public bin_op<utils::plus_mod<P>, P, Domain, log_val_traits<P>::undefined> {};

    template<std::size_t P, domain_e Domain = domain_e::nonhomogenous>
    struct MULTIPLIES_MOD : public bin_op<utils::multiplies_mod<P>, P, Domain, log_val_traits<P>::undefined> {};
}

#endif