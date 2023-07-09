#ifndef LIBTEDDY_DETAILS_OPERATORS_HPP
#define LIBTEDDY_DETAILS_OPERATORS_HPP

#include <libteddy/details/types.hpp>
#include <libteddy/details/tools.hpp>

#include <concepts>

// TODO details namespace
namespace teddy
{
namespace details
{
template<int32 M>
struct plus_mod_t
{
    template<class... Args>
    auto constexpr operator()(Args... args) const noexcept
    {
        return (args + ...) % M;
    }
};

template<int32 M>
struct multiplies_mod_t
{
    template<class... Args>
    auto constexpr operator()(Args... args) const noexcept
    {
        return (args * ...) % M;
    }
};

struct logical_and_t
{
    template<class... Args>
    auto constexpr operator()(Args... args) const noexcept
    {
        return (args && ...);
    }
};

struct logical_or_t
{
    template<class... Args>
    auto constexpr operator()(Args... args) const noexcept
    {
        return (args || ...);
    }
};

struct logical_nand_t
{
    template<class... Args>
    auto constexpr operator()(Args... args) const noexcept
    {
        return not (args && ...);
    }
};

struct logical_nor_t
{
    template<class... Args>
    auto constexpr operator()(Args... args) const noexcept
    {
        return not (args || ...);
    }
};

struct logical_xor_t
{
    template<class... Args>
    auto constexpr operator()(Args... args) const noexcept
    {
        return (args != ...);
    }
};

struct equal_to_t
{
    template<class... Args>
    auto constexpr operator()(Args... args) const noexcept
    {
        return (args == ...);
    }
};

struct not_equal_to_t
{
    template<class... Args>
    auto constexpr operator()(Args... args) const noexcept
    {
        return (args != ...);
    }
};

struct less_t
{
    template<class T>
    auto constexpr operator() (T const lhs, T const rhs) const noexcept
    {
        return lhs < rhs;
    }
};

struct less_equal_t
{
    template<class T>
    auto constexpr operator() (T const lhs, T const rhs) const noexcept
    {
        return lhs <= rhs;
    }
};

struct greater_t
{
    template<class T>
    auto constexpr operator() (T const lhs, T const rhs) const noexcept
    {
        return lhs > rhs;
    }
};

struct greater_equal_t
{
    template<class T>
    auto constexpr operator() (T const lhs, T const rhs) const noexcept
    {
        return lhs >= rhs;
    }
};

struct min_t
{
    template<class T>
    auto constexpr operator() (T const lhs, T const rhs) const noexcept
    {
        return lhs < rhs ? lhs : rhs;
    }
};

struct max_t
{
    template<class T>
    auto constexpr operator() (T const lhs, T const rhs) const noexcept
    {
        return lhs > rhs ? lhs : rhs;
    }
};

struct minimum_t
{
    template<class X>
    auto constexpr operator()(X arg) const noexcept
    {
        return arg;
    }

    template<class X, class... Xs>
    auto constexpr operator()(X arg, Xs... args) const noexcept
    {
        return min_t()(arg, minimum_t()(args...));
    }
};

struct maximum_t
{
    template<class X>
    auto constexpr operator()(X arg) const noexcept
    {
        return arg;
    }

    template<class X, class... Xs>
    auto constexpr operator()(X arg, Xs... args) const noexcept
    {
        return max_t()(arg, maximum_t()(args...));
    }
};

struct pi_conj_t
{
    template<class T>
    auto constexpr operator() (T const lhs, T const rhs) const noexcept
    {
        return minimum_t()(lhs, rhs, Undefined);
    }
};

template<class BinOp, int32 AbsorbingVal = Undefined>
struct bin_op_base
{
    template<class... Args>
    [[nodiscard]]
    constexpr auto operator()(Args... args) const noexcept -> int32
    {
        if constexpr (AbsorbingVal != Undefined)
        {
            if (utils::any((args == AbsorbingVal) ...))
            {
                return AbsorbingVal;
            }
        }

        if (utils::any((args == Nondetermined) ...))
        {
            return Nondetermined;
        }

        return static_cast<int32>(BinOp()(args ...));
    }
};
}

/**
 *  \brief Wraps binary operation so that it can be used in \c apply .
 *  Quick fix for stateful ops.
 */
auto constexpr apply_op_wrap = [] (auto const& operation)
{
    return [operation] (auto const lhs, auto const rhs)
    {
        if (lhs == Nondetermined || rhs == Nondetermined)
        {
            return Nondetermined;
        }
        return static_cast<int32>(operation(lhs, rhs));
    };
};

/**
 *  \namespace ops
 *  \brief Contains definision of all binary operations for \c apply function
 */
namespace ops
{
struct NOT
{
};

struct AND : details::bin_op_base<details::logical_and_t, 0>
{
};

struct OR : details::bin_op_base<details::logical_or_t, 1>
{
};

struct XOR : details::bin_op_base<details::logical_xor_t>
{
};

struct PI_CONJ : details::bin_op_base<details::pi_conj_t, 0>
{
};

struct NAND : details::bin_op_base<details::logical_nand_t>
{
};

struct NOR : details::bin_op_base<details::logical_nor_t>
{
};

struct EQUAL_TO : details::bin_op_base<details::equal_to_t>
{
};

struct NOT_EQUAL_TO : details::bin_op_base<details::not_equal_to_t>
{
};

struct LESS : details::bin_op_base<details::less_t>
{
};

struct LESS_EQUAL : details::bin_op_base<details::less_equal_t>
{
};

struct GREATER : details::bin_op_base<details::greater_t>
{
};

struct GREATER_EQUAL : details::bin_op_base<details::greater_equal_t>
{
};

struct MIN : details::bin_op_base<details::minimum_t, 0>
{
};

struct MAX : details::bin_op_base<details::maximum_t>
{
};

template<int32 P>
struct PLUS : details::bin_op_base<details::plus_mod_t<P>>
{
};

template<int32 P>
struct MULTIPLIES : details::bin_op_base<details::multiplies_mod_t<P>, 0>
{
};
} // namespace ops

constexpr auto op_id (ops::AND)
{
    return int32 {0};
}

constexpr auto op_id (ops::OR)
{
    return int32 {1};
}

constexpr auto op_id (ops::XOR)
{
    return int32 {2};
}

constexpr auto op_id (ops::PI_CONJ)
{
    return int32 {3};
}

constexpr auto op_id (ops::NAND)
{
    return int32 {4};
}

constexpr auto op_id (ops::NOR)
{
    return int32 {5};
}

constexpr auto op_id (ops::EQUAL_TO)
{
    return int32 {6};
}

constexpr auto op_id (ops::NOT_EQUAL_TO)
{
    return int32 {7};
}

constexpr auto op_id (ops::LESS)
{
    return int32 {8};
}

constexpr auto op_id (ops::LESS_EQUAL)
{
    return int32 {9};
}

constexpr auto op_id (ops::GREATER)
{
    return int32 {10};
}

constexpr auto op_id (ops::GREATER_EQUAL)
{
    return int32 {11};
}

constexpr auto op_id (ops::MIN)
{
    return int32 {12};
}

constexpr auto op_id (ops::MAX)
{
    return int32 {13};
}

template<int32 P>
constexpr auto op_id (ops::PLUS<P>)
{
    return int32 {14};
}

template<int32 P>
constexpr auto op_id (ops::MULTIPLIES<P>)
{
    return int32 {15};
}

constexpr auto op_is_commutative (ops::AND)
{
    return true;
}

constexpr auto op_is_commutative (ops::OR)
{
    return true;
}

constexpr auto op_is_commutative (ops::XOR)
{
    return true;
}

constexpr auto op_is_commutative (ops::PI_CONJ)
{
    return true;
}

constexpr auto op_is_commutative (ops::NAND)
{
    return true;
}

constexpr auto op_is_commutative (ops::NOR)
{
    return true;
}

constexpr auto op_is_commutative (ops::EQUAL_TO)
{
    return true;
}

constexpr auto op_is_commutative (ops::NOT_EQUAL_TO)
{
    return true;
}

constexpr auto op_is_commutative (ops::LESS)
{
    return false;
}

constexpr auto op_is_commutative (ops::LESS_EQUAL)
{
    return false;
}

constexpr auto op_is_commutative (ops::GREATER)
{
    return false;
}

constexpr auto op_is_commutative (ops::GREATER_EQUAL)
{
    return false;
}

constexpr auto op_is_commutative (ops::MIN)
{
    return true;
}

constexpr auto op_is_commutative (ops::MAX)
{
    return true;
}

template<int32 P>
constexpr auto op_is_commutative (ops::PLUS<P>)
{
    return true;
}

template<int32 P>
constexpr auto op_is_commutative (ops::MULTIPLIES<P>)
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