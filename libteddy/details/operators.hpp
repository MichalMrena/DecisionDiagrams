#ifndef LIBTEDDY_DETAILS_OPERATORS_HPP
#define LIBTEDDY_DETAILS_OPERATORS_HPP

#include <libteddy/details/tools.hpp>
#include <libteddy/details/types.hpp>

namespace teddy
{
namespace details
{
template<int32 M>
struct plus_mod_t
{
    template<class... Args>
    auto constexpr operator() (Args... args) const noexcept
    {
        return (args + ...) % M;
    }
};

template<int32 M>
struct multiplies_mod_t
{
    template<class... Args>
    auto constexpr operator() (Args... args) const noexcept
    {
        return (args * ...) % M;
    }
};

struct logical_and_t
{
    template<class... Args>
    auto constexpr operator() (Args... args) const noexcept
    {
        return (args && ...);
    }
};

struct logical_or_t
{
    template<class... Args>
    auto constexpr operator() (Args... args) const noexcept
    {
        return (args || ...);
    }
};

struct logical_nand_t
{
    template<class... Args>
    auto constexpr operator() (Args... args) const noexcept
    {
        return not (args && ...);
    }
};

struct logical_nor_t
{
    template<class... Args>
    auto constexpr operator() (Args... args) const noexcept
    {
        return not (args || ...);
    }
};

struct logical_xor_t
{
    template<class... Args>
    auto constexpr operator() (Args... args) const noexcept
    {
        return (args != ...);
    }
};

struct implies_t
{
    template<class T>
    auto constexpr operator() (T const lhs, T const rhs) const noexcept
    {
        return not lhs || rhs;
    }
};

struct equal_to_t
{
    template<class... Args>
    auto constexpr operator() (Args... args) const noexcept
    {
        return (args == ...);
    }
};

struct not_equal_to_t
{
    template<class... Args>
    auto constexpr operator() (Args... args) const noexcept
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
    auto constexpr operator() (X arg) const noexcept
    {
        return arg;
    }

    template<class X, class... Xs>
    auto constexpr operator() (X arg, Xs... args) const noexcept
    {
        return min_t()(arg, minimum_t()(args...));
    }
};

struct maximum_t
{
    template<class X>
    auto constexpr operator() (X arg) const noexcept
    {
        return arg;
    }

    template<class X, class... Xs>
    auto constexpr operator() (X arg, Xs... args) const noexcept
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
struct operation_base
{
    template<class... Args>
    [[nodiscard]] constexpr auto operator() (Args... args) const noexcept
        -> int32
    {
        if constexpr (AbsorbingVal != Undefined)
        {
            if (utils::any((args == AbsorbingVal)...))
            {
                return AbsorbingVal;
            }
        }

        if (utils::any((args == Nondetermined)...))
        {
            return Nondetermined;
        }

        return static_cast<int32>(BinOp()(args...));
    }
};
} // namespace details

/**
 *  \namespace ops
 *  \brief Contains definision of all binary operations for \c apply function
 */
namespace ops
{
struct NOT
{
};

struct AND : details::operation_base<details::logical_and_t, 0>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 1;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return true;
    }
};

struct OR : details::operation_base<details::logical_or_t, 1>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 2;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return true;
    }
};

struct XOR : details::operation_base<details::logical_xor_t>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 3;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return true;
    }
};

struct PI_CONJ : details::operation_base<details::pi_conj_t, 0>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 4;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return true;
    }
};

struct NAND : details::operation_base<details::logical_nand_t>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 5;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return true;
    }
};

struct NOR : details::operation_base<details::logical_nor_t>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 6;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return true;
    }
};

struct EQUAL_TO : details::operation_base<details::equal_to_t>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 7;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return true;
    }
};

struct NOT_EQUAL_TO : details::operation_base<details::not_equal_to_t>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 8;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return true;
    }
};

struct LESS : details::operation_base<details::less_t>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 9;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return false;
    }
};

struct LESS_EQUAL : details::operation_base<details::less_equal_t>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 10;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return false;
    }
};

struct GREATER : details::operation_base<details::greater_t>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 11;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return false;
    }
};

struct GREATER_EQUAL : details::operation_base<details::greater_equal_t>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 12;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return false;
    }
};

struct MIN : details::operation_base<details::minimum_t, 0>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 13;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return true;
    }
};

struct MAX : details::operation_base<details::maximum_t>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 14;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return true;
    }
};

template<int32 P>
struct PLUS : details::operation_base<details::plus_mod_t<P>>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 15;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return true;
    }
};

template<int32 P>
struct MULTIPLIES : details::operation_base<details::multiplies_mod_t<P>, 0>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 16;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return true;
    }
};

struct IMPLIES : details::operation_base<details::implies_t>
{
    [[nodiscard]] static auto constexpr get_id() noexcept -> int32
    {
        return 17;
    }

    [[nodiscard]] static auto constexpr is_commutative() noexcept -> bool
    {
        return false;
    }
};
} // namespace ops

template<class Operation>
concept teddy_bin_op = requires() {
                           {
                               Operation::get_id()
                           } -> utils::same_as<int32>;
                           {
                               Operation::is_commutative()
                           } -> utils::same_as<bool>;
                       };
} // namespace teddy

#endif