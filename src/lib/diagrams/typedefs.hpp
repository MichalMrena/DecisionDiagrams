#ifndef _MIX_DD_TYPEDEFS_
#define _MIX_DD_TYPEDEFS_

#include <cstdint>
#include <cmath>
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

        inline static constexpr auto undefined     = type {P};
        inline static constexpr auto nondetermined = type {P + 1};
        inline static constexpr auto valuecount    = type {P + 1};
    };
    
    using bool_t = typename log_val_traits<2>::type;

    template<std::size_t P>
    constexpr auto is_undefined(typename log_val_traits<P>::type v)
    {
        return log_val_traits<2>::undefined == v;
    }

    template<std::size_t P>
    constexpr auto is_nondetermined(typename log_val_traits<P>::type v)
    {
        return log_val_traits<2>::nondetermined == v;
    }
}

#endif