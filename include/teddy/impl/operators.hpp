#ifndef MIX_DD_OPERATORS_HPP
#define MIX_DD_OPERATORS_HPP

#include "types.hpp"
#include <type_traits>

namespace teddy
{
    template<uint_t M>
    struct plus_mod_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return (l + r) % M;
        }
    };

    template<uint_t M>
    struct multiplies_mod_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return (l * r) % M;
        }
    };

    struct logical_and_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return l and r;
        }
    };

    struct logical_or_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return l or r;
        }
    };

    struct logical_nand_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return not (l and r);
        }
    };

    struct logical_nor_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return not (l or r);
        }
    };

    struct logical_xor_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return l != r;
        }
    };

    struct equal_to_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return l == r;
        }
    };

    struct not_equal_to_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return l != r;
        }
    };

    struct less_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return l < r;
        }
    };

    struct less_equal_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return l <= r;
        }
    };

    struct greater_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return l > r;
        }
    };

    struct greater_equal_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return l >= r;
        }
    };

    struct min_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return l < r ? l : r;
        }
    };

    struct max_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return l > r ? l : r;
        }
    };

    struct pi_conj_t
    {
        template<class T>
        auto constexpr operator() (T const l, T const r) const noexcept
        {
            return min_t()(min_t()(l, r), Undefined);
        }
    };

    template<class BinOp, uint_t AbsorbingVal = Undefined>
    struct bin_op_base
    {
        [[nodiscard]] constexpr auto operator()
            (uint_t const lhs, uint_t const rhs) const noexcept -> uint_t
        {
            if constexpr (AbsorbingVal != Undefined)
            {
                if (AbsorbingVal == lhs || AbsorbingVal == rhs)
                {
                    return AbsorbingVal;
                }
            }

            if (lhs == Nondetermined || rhs == Nondetermined)
            {
                return Nondetermined;
            }

            // return static_cast<uint_t>(BinOp () (lhs, rhs));
            return BinOp()(lhs, rhs);
        }
    };

    namespace ops
    {
        struct NOT {};

        struct AND : public bin_op_base<logical_and_t, 0> {};

        struct OR : public bin_op_base< logical_or_t, 1> {};

        struct XOR : public bin_op_base<logical_xor_t> {};

        struct PI_CONJ : public bin_op_base<pi_conj_t, 0> {};

        struct NAND : public bin_op_base<logical_nand_t> {};

        struct NOR : public bin_op_base<logical_nor_t> {};

        struct EQUAL_TO : public bin_op_base<equal_to_t> {};

        struct NOT_EQUAL_TO : public bin_op_base<not_equal_to_t> {};

        struct LESS : public bin_op_base<less_t> {};

        struct LESS_EQUAL : public bin_op_base<less_equal_t> {};

        struct GREATER : public bin_op_base<greater_t> {};

        struct GREATER_EQUAL : public bin_op_base<greater_equal_t> {};

        struct MIN : public bin_op_base<min_t, 0> {};

        struct MAX : public bin_op_base<max_t> {};

        template<uint_t P>
        struct PLUS : public bin_op_base<plus_mod_t<P>> {};

        template<uint_t P>
        struct MULTIPLIES : public bin_op_base<multiplies_mod_t<P>, 0> {};
    }

    constexpr auto op_id (ops::AND)           { return uint_t {0};  }
    constexpr auto op_id (ops::OR)            { return uint_t {1};  }
    constexpr auto op_id (ops::XOR)           { return uint_t {2};  }
    constexpr auto op_id (ops::PI_CONJ)       { return uint_t {3};  }
    constexpr auto op_id (ops::NAND)          { return uint_t {4};  }
    constexpr auto op_id (ops::NOR)           { return uint_t {5};  }
    constexpr auto op_id (ops::EQUAL_TO)      { return uint_t {6};  }
    constexpr auto op_id (ops::NOT_EQUAL_TO)  { return uint_t {7};  }
    constexpr auto op_id (ops::LESS)          { return uint_t {8};  }
    constexpr auto op_id (ops::LESS_EQUAL)    { return uint_t {9};  }
    constexpr auto op_id (ops::GREATER)       { return uint_t {10}; }
    constexpr auto op_id (ops::GREATER_EQUAL) { return uint_t {11}; }
    constexpr auto op_id (ops::MIN)           { return uint_t {12}; }
    constexpr auto op_id (ops::MAX)           { return uint_t {13}; }
    template<uint_t P>
    constexpr auto op_id (ops::PLUS<P>)       { return uint_t {14}; }
    template<uint_t P>
    constexpr auto op_id (ops::MULTIPLIES<P>) { return uint_t {15}; }

    constexpr auto op_is_commutative (ops::AND)           { return true;  }
    constexpr auto op_is_commutative (ops::OR)            { return true;  }
    constexpr auto op_is_commutative (ops::XOR)           { return true;  }
    constexpr auto op_is_commutative (ops::PI_CONJ)       { return true;  }
    constexpr auto op_is_commutative (ops::NAND)          { return true;  }
    constexpr auto op_is_commutative (ops::NOR)           { return true;  }
    constexpr auto op_is_commutative (ops::EQUAL_TO)      { return true;  }
    constexpr auto op_is_commutative (ops::NOT_EQUAL_TO)  { return true;  }
    constexpr auto op_is_commutative (ops::LESS)          { return false; }
    constexpr auto op_is_commutative (ops::LESS_EQUAL)    { return false; }
    constexpr auto op_is_commutative (ops::GREATER)       { return false; }
    constexpr auto op_is_commutative (ops::GREATER_EQUAL) { return false; }
    constexpr auto op_is_commutative (ops::MIN)           { return true;  }
    constexpr auto op_is_commutative (ops::MAX)           { return true;  }
    template<uint_t P>
    constexpr auto op_is_commutative (ops::PLUS<P>)       { return true;  }
    template<uint_t P>
    constexpr auto op_is_commutative (ops::MULTIPLIES<P>) { return true;  }

    template<class O>
    concept bin_op = requires (O o)
    {
        { op_id(o) } -> std::convertible_to<uint_t>;
        { op_is_commutative(o) } -> std::same_as<bool>;
    };
}

#endif