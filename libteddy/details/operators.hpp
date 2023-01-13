#ifndef LIBTEDDY_DETAILS_OPERATORS_HPP
#define LIBTEDDY_DETAILS_OPERATORS_HPP

#include <concepts>
#include <libteddy/details/types.hpp>

namespace teddy
{
template<int32 M>
struct plus_mod_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return (l + r) % M;
    }
};

template<int32 M>
struct multiplies_mod_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return (l * r) % M;
    }
};

struct logical_and_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return l and r;
    }
};

struct logical_or_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return l or r;
    }
};

struct logical_nand_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return not (l and r);
    }
};

struct logical_nor_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return not (l or r);
    }
};

struct logical_xor_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return l != r;
    }
};

struct equal_to_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return l == r;
    }
};

struct not_equal_to_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return l != r;
    }
};

struct less_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return l < r;
    }
};

struct less_equal_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return l <= r;
    }
};

struct greater_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return l > r;
    }
};

struct greater_equal_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return l >= r;
    }
};

struct min_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return l < r ? l : r;
    }
};

struct max_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return l > r ? l : r;
    }
};

struct pi_conj_t
{
    template<class T>
    auto constexpr operator()(T const l, T const r) const noexcept
    {
        return min_t()(min_t()(l, r), Undefined);
    }
};

template<class BinOp, int32 AbsorbingVal = Undefined>
struct bin_op_base
{
    [[nodiscard]] constexpr auto operator()(int32 const lhs, int32 const rhs)
        const noexcept -> int32
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

        return static_cast<int32>(BinOp()(lhs, rhs));
    }
};

/**
 *  \brief Wraps binary operation so that it can be used in \c apply .
 *  Quick fix for stateful ops.
 */
auto constexpr apply_op_wrap = [](auto const& op)
{
    return [op](auto const l, auto const r)
    {
        if (l == Nondetermined || r == Nondetermined)
        {
            return Nondetermined;
        }
        return static_cast<int32>(op(l, r));
    };
};

/**
 *  \namespace ops
 *  \brief Contains definision of all binary operations that can be
 *  used in the \c apply function.
 */
namespace ops
{
struct NOT
{
};

struct AND : bin_op_base<logical_and_t, 0>
{
};

struct OR : bin_op_base<logical_or_t, 1>
{
};

struct XOR : bin_op_base<logical_xor_t>
{
};

struct PI_CONJ : bin_op_base<pi_conj_t, 0>
{
};

struct NAND : bin_op_base<logical_nand_t>
{
};

struct NOR : bin_op_base<logical_nor_t>
{
};

struct EQUAL_TO : bin_op_base<equal_to_t>
{
};

struct NOT_EQUAL_TO : bin_op_base<not_equal_to_t>
{
};

struct LESS : bin_op_base<less_t>
{
};

struct LESS_EQUAL : bin_op_base<less_equal_t>
{
};

struct GREATER : bin_op_base<greater_t>
{
};

struct GREATER_EQUAL : bin_op_base<greater_equal_t>
{
};

struct MIN : bin_op_base<min_t, 0>
{
};

struct MAX : bin_op_base<max_t>
{
};

template<int32 P>
struct PLUS : bin_op_base<plus_mod_t<P>>
{
};

template<int32 P>
struct MULTIPLIES : bin_op_base<multiplies_mod_t<P>, 0>
{
};
} // namespace ops

constexpr auto op_id(ops::AND)
{
    return int32 {0};
}
constexpr auto op_id(ops::OR)
{
    return int32 {1};
}
constexpr auto op_id(ops::XOR)
{
    return int32 {2};
}
constexpr auto op_id(ops::PI_CONJ)
{
    return int32 {3};
}
constexpr auto op_id(ops::NAND)
{
    return int32 {4};
}
constexpr auto op_id(ops::NOR)
{
    return int32 {5};
}
constexpr auto op_id(ops::EQUAL_TO)
{
    return int32 {6};
}
constexpr auto op_id(ops::NOT_EQUAL_TO)
{
    return int32 {7};
}
constexpr auto op_id(ops::LESS)
{
    return int32 {8};
}
constexpr auto op_id(ops::LESS_EQUAL)
{
    return int32 {9};
}
constexpr auto op_id(ops::GREATER)
{
    return int32 {10};
}
constexpr auto op_id(ops::GREATER_EQUAL)
{
    return int32 {11};
}
constexpr auto op_id(ops::MIN)
{
    return int32 {12};
}
constexpr auto op_id(ops::MAX)
{
    return int32 {13};
}
template<int32 P>
constexpr auto op_id(ops::PLUS<P>)
{
    return int32 {14};
}
template<int32 P>
constexpr auto op_id(ops::MULTIPLIES<P>)
{
    return int32 {15};
}

constexpr auto op_is_commutative(ops::AND)
{
    return true;
}
constexpr auto op_is_commutative(ops::OR)
{
    return true;
}
constexpr auto op_is_commutative(ops::XOR)
{
    return true;
}
constexpr auto op_is_commutative(ops::PI_CONJ)
{
    return true;
}
constexpr auto op_is_commutative(ops::NAND)
{
    return true;
}
constexpr auto op_is_commutative(ops::NOR)
{
    return true;
}
constexpr auto op_is_commutative(ops::EQUAL_TO)
{
    return true;
}
constexpr auto op_is_commutative(ops::NOT_EQUAL_TO)
{
    return true;
}
constexpr auto op_is_commutative(ops::LESS)
{
    return false;
}
constexpr auto op_is_commutative(ops::LESS_EQUAL)
{
    return false;
}
constexpr auto op_is_commutative(ops::GREATER)
{
    return false;
}
constexpr auto op_is_commutative(ops::GREATER_EQUAL)
{
    return false;
}
constexpr auto op_is_commutative(ops::MIN)
{
    return true;
}
constexpr auto op_is_commutative(ops::MAX)
{
    return true;
}
template<int32 P>
constexpr auto op_is_commutative(ops::PLUS<P>)
{
    return true;
}
template<int32 P>
constexpr auto op_is_commutative(ops::MULTIPLIES<P>)
{
    return true;
}

template<class O>
concept bin_op = requires(O o) {
                     {
                         op_id(o)
                     } -> std::convertible_to<int32>;
                     {
                         op_is_commutative(o)
                     } -> std::same_as<bool>;
                 };
} // namespace teddy

#endif