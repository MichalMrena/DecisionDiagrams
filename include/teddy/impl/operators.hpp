#ifndef MIX_DD_OPERATORS_HPP
#define MIX_DD_OPERATORS_HPP

#include "types.hpp"

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
        };
    };

    template<class BinOp, uint_t AbsorbingVal = Undefined>
    struct bin_op
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

    struct NOT {};

    struct AND : public bin_op<logical_and_t, 0> {};

    struct OR : public bin_op< logical_or_t, 1> {};

    struct XOR : public bin_op<logical_xor_t> {};

    struct PI_CONJ : public bin_op<pi_conj_t, 0> {};

    struct NAND : public bin_op<logical_nand_t> {};

    struct NOR : public bin_op<logical_nor_t> {};

    struct EQUAL_TO : public bin_op<equal_to_t> {};

    struct NOT_EQUAL_TO : public bin_op<not_equal_to_t> {};

    struct LESS : public bin_op<less_t> {};

    struct LESS_EQUAL : public bin_op<less_equal_t> {};

    struct GREATER : public bin_op<greater_t> {};

    struct GREATER_EQUAL : public bin_op<greater_equal_t> {};

    struct MIN : public bin_op<min_t, 0> {};

    struct MAX : public bin_op<max_t> {};

    template<uint_t P>
    struct PLUS : public bin_op<plus_mod_t<P>> {};

    template<uint_t P>
    struct MULTIPLIES : public bin_op<multiplies_mod_t<P>, 0> {};

    constexpr auto op_id (AND)           { return uint_t {0};  }
    constexpr auto op_id (OR)            { return uint_t {1};  }
    constexpr auto op_id (XOR)           { return uint_t {2};  }
    constexpr auto op_id (PI_CONJ)       { return uint_t {3};  }
    constexpr auto op_id (NAND)          { return uint_t {4};  }
    constexpr auto op_id (NOR)           { return uint_t {5};  }
    constexpr auto op_id (EQUAL_TO)      { return uint_t {6};  }
    constexpr auto op_id (NOT_EQUAL_TO)  { return uint_t {7};  }
    constexpr auto op_id (LESS)          { return uint_t {8};  }
    constexpr auto op_id (LESS_EQUAL)    { return uint_t {9};  }
    constexpr auto op_id (GREATER)       { return uint_t {10}; }
    constexpr auto op_id (GREATER_EQUAL) { return uint_t {11}; }
    constexpr auto op_id (MIN)           { return uint_t {12}; }
    constexpr auto op_id (MAX)           { return uint_t {13}; }
    template<uint_t P>
    constexpr auto op_id (PLUS<P>)       { return uint_t {14}; }
    template<uint_t P>
    constexpr auto op_id (MULTIPLIES<P>) { return uint_t {15}; }

    constexpr auto op_is_commutative (AND)           { return true;  }
    constexpr auto op_is_commutative (OR)            { return true;  }
    constexpr auto op_is_commutative (XOR)           { return true;  }
    constexpr auto op_is_commutative (PI_CONJ)       { return true;  }
    constexpr auto op_is_commutative (NAND)          { return true;  }
    constexpr auto op_is_commutative (NOR)           { return true;  }
    constexpr auto op_is_commutative (EQUAL_TO)      { return true;  }
    constexpr auto op_is_commutative (NOT_EQUAL_TO)  { return true;  }
    constexpr auto op_is_commutative (LESS)          { return false; }
    constexpr auto op_is_commutative (LESS_EQUAL)    { return false; }
    constexpr auto op_is_commutative (GREATER)       { return false; }
    constexpr auto op_is_commutative (GREATER_EQUAL) { return false; }
    constexpr auto op_is_commutative (MIN)           { return true;  }
    constexpr auto op_is_commutative (MAX)           { return true;  }
    template<uint_t P>
    constexpr auto op_is_commutative (PLUS<P>)       { return true;  }
    template<uint_t P>
    constexpr auto op_is_commutative (MULTIPLIES<P>) { return true;  }

    inline constexpr auto OpCount = std::size_t {16};
}

#endif