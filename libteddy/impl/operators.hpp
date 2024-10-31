#ifndef LIBTEDDY_DETAILS_OPERATORS_HPP
#define LIBTEDDY_DETAILS_OPERATORS_HPP

#include <libteddy/impl/tools.hpp>
#include <libteddy/impl/types.hpp>

namespace teddy
{
namespace details
{
  template<class Op>
  struct make_nary
  {
    template<class... Ts>
    [[nodiscard]]
    auto constexpr
      operator() (int32 const t, Ts... ts) const -> int32
    {
      Op const& op = static_cast<Op const&>(*this);
      return op(t, op(ts...));
    }
  };

  template<int32 Id, bool IsCommutative>
  struct operation_info
  {
    [[nodiscard]] static auto constexpr get_id() -> int32
    {
      return Id;
    }

    [[nodiscard]] static auto constexpr is_commutative() -> bool
    {
      return IsCommutative;
    }
  };

} // namespace details

/**
 *  \namespace ops
 *  \brief Contains definitions of all binary operations for \c apply function
 */
namespace ops
{
  struct AND : details::make_nary<AND>, details::operation_info<1, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const mi = utils::min(l, r);
      int32 const ma = utils::max(l, r);
      return mi == 0 ? mi : ma;
    }

    using details::make_nary<AND>::operator();
  };

  struct OR : details::make_nary<OR>, details::operation_info<2, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int const mi = utils::min(l, r);
      int const ma = utils::max(l, r);
      return mi == 0 ? ma : mi;
    }

    using details::make_nary<OR>::operator();
  };

  struct XOR : details::make_nary<XOR>, details::operation_info<3, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int const xi = l ^ r;
      int const ma = utils::max(l, r);
      return ma == Nondetermined ? ma : xi;
    }

    using details::make_nary<XOR>::operator();
  };

  struct PI_CONJ : details::make_nary<PI_CONJ>, details::operation_info<4, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const mi = utils::min(l, r);
      int32 const ma = utils::max(l, r);
      return mi == 0 ? mi : ma == Undefined ? mi : ma;
    }

    using details::make_nary<PI_CONJ>::operator();
  };

  struct NAND : details::make_nary<NAND>, details::operation_info<5, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const mi = utils::min(l, r);
      int32 const ma = utils::max(l, r);
      return ma == Nondetermined ? ma : 1 - mi;
    }

    using details::make_nary<NAND>::operator();
  };

  struct NOR : details::make_nary<NOR>, details::operation_info<6, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      // This assumes that l,r is from {0,1,N} where N has 0 at lowest
      // bit.

      int32 const mi  = utils::min(l, r);
      int32 const ma  = utils::max(l, r);
      int32 const ema = utils::max(l | r, 1);
      return (mi & 1) | (ma & 1) ? 0 : ema;
    }

    using details::make_nary<NOR>::operator();
  };

  struct XNOR : details::make_nary<XNOR>, details::operation_info<7, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const ma = utils::max(l, r);
      int32 const ne = static_cast<int32>(l == r);
      return ma == Nondetermined ? ma : ne;
    }

    using details::make_nary<XNOR>::operator();
  };

  struct EQUAL_TO :
    details::make_nary<EQUAL_TO>,
    details::operation_info<8, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const ma = utils::max(l, r);
      int32 const eq = static_cast<int32>(l == r);
      return ma == Nondetermined ? ma : eq;
    }

    using details::make_nary<EQUAL_TO>::operator();
  };

  struct NOT_EQUAL_TO :
    details::make_nary<NOT_EQUAL_TO>,
    details::operation_info<9, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const ma = utils::max(l, r);
      int32 const ne = static_cast<int32>(l != r);
      return ma == Nondetermined ? ma : ne;
    }

    using details::make_nary<NOT_EQUAL_TO>::operator();
  };

  struct LESS : details::operation_info<10, false>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const ma = utils::max(l, r);
      int32 const le = static_cast<int32>(l < r);
      return ma == Nondetermined ? ma : le;
    }
  };

  struct LESS_EQUAL : details::operation_info<11, false>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const ma = utils::max(l, r);
      int32 const le = static_cast<int32>(l <= r);
      return ma == Nondetermined ? ma : le;
    }
  };

  struct GREATER : details::operation_info<12, false>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const ma = utils::max(l, r);
      int32 const ge = static_cast<int32>(l > r);
      return ma == Nondetermined ? ma : ge;
    }
  };

  struct GREATER_EQUAL : details::operation_info<13, false>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const ma = utils::max(l, r);
      int32 const ge = static_cast<int32>(l >= r);
      // return l == 0 ? 1 : ma == Nondetermined ? Nondetermined : ge;
      return ma == Nondetermined ? Nondetermined : ge;
    }
  };

  struct MIN : details::make_nary<MIN>, details::operation_info<14, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const mi = utils::min(l, r);
      int32 const ma = utils::max(l, r);
      return mi == 0 || ma != Nondetermined ? mi : ma;
    }

    using details::make_nary<MIN>::operator();
  };

  struct MAX : details::make_nary<MAX>, details::operation_info<15, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      return utils::max(l, r);
    }

    using details::make_nary<MAX>::operator();
  };

  /**
   *  \brief Same as \c MAX but short-circuits for \p M -- should be faster
   *  \tparam M maximum of the domain of possible values
   */
  template<int32 M>
  struct MAXB : details::make_nary<MAXB<M>>, details::operation_info<16, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const ma = utils::max(l, r);
      return l == M - 1 || r == M - 1 ? M - 1 : ma;
    }

    using details::make_nary<MAXB>::operator();
  };

  template<int32 M>
  struct PLUS : details::make_nary<PLUS<M>>, details::operation_info<17, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const ma = utils::max(l, r);
      int32 const pl = (l + r) % M;
      return ma == Nondetermined ? ma : pl;
    }
  };

  template<int32 M>
  struct MULTIPLIES :
    details::make_nary<MULTIPLIES<M>>,
    details::operation_info<18, true>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      int32 const mi = utils::min(l, r);
      int32 const ma = utils::max(l, r);
      int32 const ml = (l * r) % M;
      return mi == 0 ? 0 : ma == Nondetermined ? Nondetermined : ml;
    }
  };

  struct IMPLIES : details::operation_info<19, false>
  {
    [[nodiscard]]
    auto constexpr
      operator() (int32 const l, int32 const r) const -> int32
    {
      // +---+---+--------+
      // | l | r | l => r |
      // +-------+--------+
      // | 0 | 0 |   1    |
      // | 0 | 1 |   1    |  if l == 0 return 1
      // | 0 | N |   1    |
      // +-------+--------+
      // | 1 | 0 |   0    |
      // | 1 | 1 |   1    |  elsif l == 1 return r
      // | 1 | N |   N    |
      // +-------+--------+
      // | N | 0 |   N    |
      // | N | 1 |   1    |  elsif r == 1 return 1
      // | N | N |   N    |  else  return Nondetermined
      // +-------+--------+

      bool const l0     = l == 0;
      bool const l1     = l == 1;
      bool const r1     = r == 1;
      int32 const r1res = r1 ? 1 : Nondetermined;
      int32 const l1res = l1 ? r : r1res;
      return l0 ? 1 : l1res;
    }
  };
} // namespace ops

template<class Operation>
concept teddy_bin_op = requires() {
  { Operation::get_id() } -> utils::same_as<int32>;
  { Operation::is_commutative() } -> utils::same_as<bool>;
};
} // namespace teddy

#endif