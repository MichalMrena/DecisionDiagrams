#ifndef MIX_DD_TYPEDEFS_HPP
#define MIX_DD_TYPEDEFS_HPP

#include <cstdint>
#include <limits>
#include <string>

namespace mix::dd
{
    using var_vals_t = std::uint64_t;
    using id_t       = std::int32_t;
    using index_t    = std::uint32_t;

    template<std::size_t P>
    struct log_val_traits
    {
        using type = std::uint8_t;

        static_assert(P < std::numeric_limits<type>::max(), "Max for P is 254.");

        inline static constexpr auto undefined     = type {P};      // * in extended dpbds
        inline static constexpr auto nondetermined = type {P + 1};  // internal vertex in apply
        inline static constexpr auto nodomain      = type {P + 2};  // non homogenous functions
        inline static constexpr auto valuecount    = type {P + 2};
    };

    using bool_t = typename log_val_traits<2>::type;

    template<std::size_t P>
    constexpr auto is_undefined(typename log_val_traits<P>::type const v)
    {
        return log_val_traits<P>::undefined == v;
    }

    template<std::size_t P>
    constexpr auto is_nondetermined(typename log_val_traits<P>::type const v)
    {
        return log_val_traits<P>::nondetermined == v;
    }

    template<std::size_t P>
    constexpr auto is_nodomain(typename log_val_traits<P>::type const v)
    {
        return log_val_traits<P>::nodomain == v;
    }
}

#endif