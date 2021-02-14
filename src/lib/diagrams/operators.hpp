#ifndef MIX_DD_OPERATORS_HPP
#define MIX_DD_OPERATORS_HPP

#include "typedefs.hpp"
#include "../utils/more_functional.hpp"

#include <algorithm>
#include <functional>
#include <limits>

namespace mix::dd
{
    namespace impl
    {
        template<std::size_t P>
        inline constexpr auto U = log_val_traits<P>::undefined;

        template<std::size_t P>
        inline auto constexpr pi_conj = [](auto const l, auto const r) { return std::min({l, r, U<P>}); };
        template<std::size_t P>
        using pi_conj_t = decltype(pi_conj<P>);

        /**
            @brief Base class for binary operators.

            Wrapper for binary operations that are used in the `apply`.
            Handles these cases for all operations in this order:
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

            @tparam BinOp stateless binary operation.
            @tparam P domain of the function.
            @tparam Domain specifies whether @p lhs or @p rhs can be `nodomain`.
            @tparam AbsorbingVal the value if @p BinOp has it or `undefined`.
        */
        template<class BinOp, std::size_t P, auto AbsorbingVal>
        struct bin_op
        {
            using log_t = typename log_val_traits<P>::type;

            [[nodiscard]] constexpr auto operator()
                (log_t const lhs, log_t const rhs) const noexcept (BinOp () (log_t{}, log_t{})) -> log_t
            {
                if constexpr (P > 2)
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

                return static_cast<log_t>(BinOp () (lhs, rhs));
            }
        };
    }

    struct NOT {};

    template<std::size_t P> struct AND           : public impl::bin_op< std::logical_and<>,         P, 0          > {};
    template<std::size_t P> struct OR            : public impl::bin_op< std::logical_or<>,          P, 1          > {};
    template<std::size_t P> struct XOR           : public impl::bin_op< std::not_equal_to<>,        P, impl::U<P> > {};
    template<std::size_t P> struct PI_CONJ       : public impl::bin_op< impl::pi_conj_t<P>,         P, 0          > {};
    template<std::size_t P> struct NAND          : public impl::bin_op< utils::logical_nand_t,      P, impl::U<P> > {};
    template<std::size_t P> struct NOR           : public impl::bin_op< utils::logical_nor_t,       P, impl::U<P> > {};
    template<std::size_t P> struct EQUAL_TO      : public impl::bin_op< std::equal_to<>,            P, impl::U<P> > {};
    template<std::size_t P> struct NOT_EQUAL_TO  : public impl::bin_op< std::not_equal_to<>,        P, impl::U<P> > {};
    template<std::size_t P> struct LESS          : public impl::bin_op< std::less<>,                P, impl::U<P> > {};
    template<std::size_t P> struct LESS_EQUAL    : public impl::bin_op< std::less_equal<>,          P, impl::U<P> > {};
    template<std::size_t P> struct GREATER       : public impl::bin_op< std::greater<>,             P, impl::U<P> > {};
    template<std::size_t P> struct GREATER_EQUAL : public impl::bin_op< std::greater_equal<>,       P, impl::U<P> > {};
    template<std::size_t P> struct MIN           : public impl::bin_op< utils::min_t,               P, 0          > {};
    template<std::size_t P> struct MAX           : public impl::bin_op< utils::max_t,               P, P - 1      > {};
    template<std::size_t P> struct PLUS          : public impl::bin_op< utils::plus_mod_t<P>,       P, impl::U<P> > {};
    template<std::size_t P> struct MULTIPLIES    : public impl::bin_op< utils::multiplies_mod_t<P>, P, 0          > {};

    /**
        @brief Type of unique identifier of operations.
     */
    using op_id_t = std::uint8_t;

    auto constexpr op_count () { return std::size_t {16}; }

    /**
        @brief Maps operations to their integer ids. Id is used in the apply cache.
     */
    template<std::size_t P> constexpr auto op_id (AND<P>)           { return op_id_t {0};  }
    template<std::size_t P> constexpr auto op_id (OR<P>)            { return op_id_t {1};  }
    template<std::size_t P> constexpr auto op_id (XOR<P>)           { return op_id_t {2};  }
    template<std::size_t P> constexpr auto op_id (PI_CONJ<P>)       { return op_id_t {3};  }
    template<std::size_t P> constexpr auto op_id (NAND<P>)          { return op_id_t {4};  }
    template<std::size_t P> constexpr auto op_id (NOR<P>)           { return op_id_t {5};  }
    template<std::size_t P> constexpr auto op_id (EQUAL_TO<P>)      { return op_id_t {6};  }
    template<std::size_t P> constexpr auto op_id (NOT_EQUAL_TO<P>)  { return op_id_t {7};  }
    template<std::size_t P> constexpr auto op_id (LESS<P>)          { return op_id_t {8};  }
    template<std::size_t P> constexpr auto op_id (LESS_EQUAL<P>)    { return op_id_t {9};  }
    template<std::size_t P> constexpr auto op_id (GREATER<P>)       { return op_id_t {10}; }
    template<std::size_t P> constexpr auto op_id (GREATER_EQUAL<P>) { return op_id_t {11}; }
    template<std::size_t P> constexpr auto op_id (MIN<P>)           { return op_id_t {12}; }
    template<std::size_t P> constexpr auto op_id (MAX<P>)           { return op_id_t {13}; }
    template<std::size_t P> constexpr auto op_id (PLUS<P>)          { return op_id_t {14}; }
    template<std::size_t P> constexpr auto op_id (MULTIPLIES<P>)    { return op_id_t {15}; }

    /**
        @brief Tells whether given operation is commutative or not.
     */
    template<std::size_t P> constexpr auto op_is_commutative (AND<P>)           { return true;  }
    template<std::size_t P> constexpr auto op_is_commutative (OR<P>)            { return true;  }
    template<std::size_t P> constexpr auto op_is_commutative (XOR<P>)           { return true;  }
    template<std::size_t P> constexpr auto op_is_commutative (PI_CONJ<P>)       { return true;  }
    template<std::size_t P> constexpr auto op_is_commutative (NAND<P>)          { return true;  }
    template<std::size_t P> constexpr auto op_is_commutative (NOR<P>)           { return true;  }
    template<std::size_t P> constexpr auto op_is_commutative (EQUAL_TO<P>)      { return true;  }
    template<std::size_t P> constexpr auto op_is_commutative (NOT_EQUAL_TO<P>)  { return true;  }
    template<std::size_t P> constexpr auto op_is_commutative (LESS<P>)          { return false; }
    template<std::size_t P> constexpr auto op_is_commutative (LESS_EQUAL<P>)    { return false; }
    template<std::size_t P> constexpr auto op_is_commutative (GREATER<P>)       { return false; }
    template<std::size_t P> constexpr auto op_is_commutative (GREATER_EQUAL<P>) { return false; }
    template<std::size_t P> constexpr auto op_is_commutative (MIN<P>)           { return true;  }
    template<std::size_t P> constexpr auto op_is_commutative (MAX<P>)           { return true;  }
    template<std::size_t P> constexpr auto op_is_commutative (PLUS<P>)          { return true;  }
    template<std::size_t P> constexpr auto op_is_commutative (MULTIPLIES<P>)    { return true;  }
}

#endif